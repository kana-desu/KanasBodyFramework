#include <kbf/data/armour/armour_data_manager.hpp>

#include <kbf/data/npc/npc_prefab_alias_mappings.hpp>
#include <kbf/data/npc/npc_data_manager.hpp>

#include <kbf/util/string/to_lower.hpp>
#include <kbf/util/string/to_binary_string.hpp>
#include <kbf/data/ids/special_armour_ids.hpp>
#include <kbf/util/re_engine/print_re_object.hpp>
#include <kbf/util/re_engine/find_transform.hpp>
#include <kbf/util/re_engine/get_component.hpp>
#include <kbf/enums/armor_parts.hpp>

#include <unordered_set>
#include <set>
#include <numeric>
#include <regex>

#define NPC_ARMOR_PREFAB_FETCH_CAP 2000
#define NPC_UNIQUE_PREFAB_SETS_FETCH_CAP 4
#define NPC_UNIQUE_PREFAB_VARIANTS_FETCH_CAP 1000

namespace kbf {

	ArmourDataManager& ArmourDataManager::get() {
		static ArmourDataManager instance;
		if (!instance.initialized) instance.initialize();

		return instance;
	}

	void ArmourDataManager::initialize() {
		if (initialized) return;

		getArmourMappings();
		initialized = true;
	}

	std::vector<ArmourSet> ArmourDataManager::getFilteredArmourSets(const std::string& filter) {
		const bool noFilter = filter.empty();
		std::string filterLower = toLower(filter);

		std::set<ArmourSet> filteredSets;
		// Hunter Sets
		for (const auto& [id, data] : armourSeriesIDMappings) {
			std::string setLower = toLower(data.name);

			if (noFilter || setLower.find(filterLower) != std::string::npos) {
				filteredSets.insert(getArmourSetFromArmourID(id));
			}
		}

		// NPC Sets
		for (const auto& [prefabPth, data] : npcPrefabToArmourSetMap) {
			std::string setLower = toLower(data.name);

			if (noFilter || setLower.find(filterLower) != std::string::npos) {
				if (data.femaleCanUse) filteredSets.insert(getArmourSetFromNpcPrefab(prefabPth, true));
				if (data.maleCanUse)   filteredSets.insert(getArmourSetFromNpcPrefab(prefabPth, false));
			}
		}

		// TODO: This is tragically inefficient
		std::vector<ArmourSet> sortedSets(filteredSets.begin(), filteredSets.end());
		std::sort(sortedSets.begin(), sortedSets.end(),
			[](const ArmourSet& a, const ArmourSet& b) {
				if (a.name == ANY_ARMOUR_ID) return true;
				if (b.name == ANY_ARMOUR_ID) return false;
				int cmp = strcmp(a.name.c_str(), b.name.c_str());
				if (cmp == 0) return !a.female;
				return cmp < 0; // a.name < b.name
			});

		return sortedSets;
	}

	ArmorSetID ArmourDataManager::getArmourSetIDFromArmourSeries(uint32_t series, bool female)
	{
		uint32_t modId = REInvokeStatic<uint32_t>(
			"app.ArmorDef",
			"ModId(app.ArmorDef.SERIES)",
			{ (void*)series },
			InvokeReturnType::DWORD);

		uint32_t modSubId = female
			? REInvokeStatic<uint32_t>("app.ArmorDef", "ModSubFemaleId(app.ArmorDef.SERIES)", { (void*)series }, InvokeReturnType::DWORD)
			: REInvokeStatic<uint32_t>("app.ArmorDef", "ModSubMaleId(app.ArmorDef.SERIES)", { (void*)series }, InvokeReturnType::DWORD);

		return { modId, modSubId };
	}


	ArmourSet ArmourDataManager::getArmourSetFromArmourID(const ArmorSetID& setId) const {
		const auto& it = armourSeriesIDMappings.find(setId);
		if (it != armourSeriesIDMappings.end()) {
			return ArmourSet{ it->second.name, it->second.female };
		}

		return ArmourSet::DEFAULT;
	}

	ArmourSet ArmourDataManager::getArmourSetFromNpcPrefab(const std::string& npcPrefabPath, bool female) const {
		const auto& it = npcPrefabToArmourSetMap.find(npcPrefabPath);
		if (it != npcPrefabToArmourSetMap.end()) {
			if (female && it->second.femaleCanUse) return ArmourSet{ it->second.name, true };
			if (!female && it->second.maleCanUse)  return ArmourSet{ it->second.name, false };
		}

		return ArmourSet::DEFAULT;
	}

	REApi::ManagedObject* ArmourDataManager::getNpcPrefabPrimaryTransform(const std::string& prefabPath, REApi::ManagedObject* baseTransform) {
		// We want to find these components WITHOUT relying on the name of them. This is because doing so is:
		//  - Slow with string checks
		//  - Unreliable if prefab names change with updates / mods
		
		auto it = npcPrefabToPrimaryTransformNameMap.find(prefabPath);
		// We know what the transform should be called, just find it
		if (it != npcPrefabToPrimaryTransformNameMap.end()) return findTransform(baseTransform, it->second);

		// We need to figure out what the primary transform is. Since this is cached, we can afford it being a touch on the expensive side.
		// Search through the character's MeshSetting instances, these live UNDER each submesh in the prefab.
		REApi::ManagedObject* baseGameObject = REInvokePtr<REApi::ManagedObject>(baseTransform, "get_GameObject", {});
		if (!baseGameObject) return nullptr;

		REApi::ManagedObject* meshSettingController = getComponent(baseGameObject, "app.MeshSettingController");
		if (!meshSettingController) return nullptr;

		REApi::ManagedObject* sequence = REInvokePtr<REApi::ManagedObject>(meshSettingController, "get_MeshSettingsAll()", {});
		if (!sequence) return nullptr;

		REApi::ManagedObject* enumerator = REInvokePtr<REApi::ManagedObject>(sequence, "System.Collections.Generic.IEnumerable<T>.GetEnumerator()", {});
		if (!enumerator) return nullptr;

		constexpr size_t FETCH_CAP = 100;
		bool canMove = REInvoke<bool>(enumerator, "MoveNext()", {}, InvokeReturnType::BOOL);
		size_t cnt = 0;

		while (canMove && cnt < FETCH_CAP) {
			REApi::ManagedObject* controller = REInvokePtr<REApi::ManagedObject>(enumerator, "System.Collections.Generic.IEnumerator<T>.get_Current()", {});
			if (controller) {
				REApi::ManagedObject* gameObj = REInvokePtr<REApi::ManagedObject>(controller, "get_GameObject", {});
				if (!gameObj) continue;

				// The main meshes just so happen to exclusively have this combination of components... might break in future.
				bool hasCharacterEditRegion         = getComponent(gameObj, "app.CharacterEditRegion");
				bool hasGroundSurfaceTrailRequester = getComponent(gameObj, "app.GroundSurfaceTrailRequester");

				if (hasCharacterEditRegion && hasGroundSurfaceTrailRequester) {
					std::string primaryTransformName = REInvokeStr(gameObj, "get_Name", {});
					npcPrefabToPrimaryTransformNameMap.emplace(prefabPath, primaryTransformName);
					return REInvokePtr<REApi::ManagedObject>(gameObj, "get_Transform", {});
				}
			}
			canMove = REInvoke<bool>(enumerator, "MoveNext()", {}, InvokeReturnType::BOOL);
			cnt++;
		}

		return nullptr;
	}

	std::string ArmourDataManager::getPartnerCostumePrefab(size_t partnerId, size_t costumeId) const {
		auto it = partnerIdToCostumePrefabMap.find(partnerId);
		if (it != partnerIdToCostumePrefabMap.end()) {
			auto costumeIt = it->second.find(costumeId);
			if (costumeIt != it->second.end()) {
				return costumeIt->second;
			}
		}
		return "";
	}

	bool ArmourDataManager::hasArmourSetMapping(const ArmourSet& set) const {
		if (knownArmourSeries.contains(set)) return true;
		if (knownNpcPrefabs.contains(set)) return true;
		return false;
	}

	ArmourPieceFlags ArmourDataManager::getResidentArmourPieces(const ArmourSet& set) const {
		if (knownNpcPrefabs.contains(set)) return ArmourPieceFlagBits::APF_BODY; // All prefabs apply under 'body' only
		if (set == ArmourSet::DEFAULT) return ArmourPieceFlagBits::APF_ALL;

		auto it = knownArmourSeries.find(set);
		if (it != knownArmourSeries.end()) {
			return armourSeriesIDMappings.at(it->second).residentPieces;
		}

		return ArmourPieceFlagBits::APF_NONE;
	}

	//bool ArmourDataManager::armourSetHasPiece(const ArmourSet& set, const ArmourPiece& piece) const {
	//	if (knownNpcPrefabs.contains(set)) return piece == ArmourPiece::AP_BODY; // All prefabs apply under 'body' only

	//	auto it = knownArmourSeries.find(set);
	//	if (it != knownArmourSeries.end()) {
	//		ArmourPieceFlags pieces = armourSeriesIDMappings.at(it->second).residentPieces;
	//		return pieces & getArmourPieceFlag(piece);
	//	}

	//	return false;
	//}

	void ArmourDataManager::getArmourMappings() {
		knownArmourSeries.clear();
		knownNpcPrefabs.clear();
		npcPrefabToPrimaryTransformNameMap.clear();
		partnerIdToCostumePrefabMap.clear();

		armourSeriesIDMappings = getArmorSeriesData();
		npcPrefabToArmourSetMap = getNpcArmorData();

		// generate resident piece mappings, and reverse lookup for quick indexing later
		for (const auto& [id, data] : armourSeriesIDMappings) {
			ArmourSet s{ data.name, data.female };
			knownArmourSeries.emplace(s, id);
		}
		for (const auto& [prefabPth, data] : npcPrefabToArmourSetMap) {
			if (data.femaleCanUse) {
				ArmourSet sf{ data.name, true };
				knownNpcPrefabs.emplace(sf, prefabPth);
			}
			if (data.maleCanUse) {
				ArmourSet sm{ data.name, false };
				knownNpcPrefabs.emplace(sm, prefabPth);
			}
		}
	}

	ArmourPieceFlags ArmourDataManager::getResidentArmourPieces(size_t armorSeries) const {
		size_t minPartIdx = static_cast<size_t>(ArmorParts::MIN);
		size_t maxPartIdx = static_cast<size_t>(ArmorParts::MAX_EXCLUDING_SLINGER);

		ArmourPieceFlags flags = ArmourPieceFlagBits::APF_NONE;
		for (size_t partIdx = minPartIdx; partIdx < maxPartIdx; partIdx++) {
			REApi::ManagedObject* armorData = REInvokeStaticPtr<REApi::ManagedObject>(
				"app.ArmorDef",
				"Data(app.ArmorDef.ARMOR_PARTS, app.ArmorDef.SERIES)",
				{ (void*)partIdx, (void*)armorSeries });

			REApi::ManagedObject* outerArmorData = REInvokeStaticPtr<REApi::ManagedObject>(
				"app.ArmorDef",
				"OuterArmorData(app.ArmorDef.ARMOR_PARTS, app.ArmorDef.SERIES)",
				{ (void*)partIdx, (void*)armorSeries });

			if (armorData || outerArmorData) {
				flags |= (1 << (1 + partIdx));
				//DEBUG_STACK.fpush<LOG_TAG>("Found piece name: {}. Flags: {}", partIdx, to_binary_string(flags));
			}
			//else {
			//	DEBUG_STACK.fpush<LOG_TAG>("No piece!");
			//}
		}

		return flags;
	}

	std::optional<ArmourSet> ArmourDataManager::getArmourSetFromPrefabName(const std::string& prefabName) {
		auto setIdOpt = getArmourSetIDFromPrefabName(prefabName);
		if (!setIdOpt.has_value()) return std::nullopt;
		ArmourSet set = getArmourSetFromArmourID(setIdOpt.value());
		if (set == ArmourSet::DEFAULT) return std::nullopt; // couldn't find a valid set for this id

		return set;
	}

	std::optional<ArmorSetID> ArmourDataManager::getArmourSetIDFromArmourSet(const ArmourSet& set) const {
		auto it = knownArmourSeries.find(set);
		if (it != knownArmourSeries.end()) {
			return it->second;
		}
		return std::nullopt;
	}

	std::optional<ArmorSetID> ArmourDataManager::getArmourSetIDFromPrefabName(const std::string& prefabName) {
		// Expects a path like ch03_XXX_YYY0_0, where 0 can be any irrelevant digit.
		// XXX gets parsed as the series id, and YYY as the subId.

		if (prefabName.size() < 13)
			return std::nullopt;

		const char* s = prefabName.data();

		// prefix "ch"
		if (s[0] != 'c' || s[1] != 'h')
			return std::nullopt;

		// two digits
		if (!std::isdigit(s[2]) || !std::isdigit(s[3]))
			return std::nullopt;

		// underscore
		if (s[4] != '_')
			return std::nullopt;

		// parse series (XXX)
		int series = 0;
		{
			auto [ptr, ec] = std::from_chars(s + 5, s + 8, series);
			if (ec != std::errc() || ptr != s + 8)
				return std::nullopt;
		}

		// underscore
		if (s[8] != '_')
			return std::nullopt;

		// parse subId (YYY)
		int subId = 0;
		{
			auto [ptr, ec] = std::from_chars(s + 9, s + 12, subId);
			if (ec != std::errc() || ptr != s + 12)
				return std::nullopt;
		}

		// must have at least one more digit after
		if (!std::isdigit(s[12]))
			return std::nullopt;

		return ArmorSetID{ static_cast<uint32_t>(series), static_cast<uint32_t>(subId) };
	}

	std::string ArmourDataManager::getPrefabNameFromArmourSetID(const ArmorSetID& setId, ArmourPiece piece, bool characterFemale) {
		// Male:   ch02_XXX_YYYP
		// Female: ch03_XXX_YYYP
		// XXX = left zero-padded setId.id, YYY = left zero-padded setId.subId
		// P = static_cast<uint32_t>(piece)
		const int gender = characterFemale ? 3 : 2;

		return std::format(
			"ch0{}_{:03}_{:03}{:01}",
			gender,
			setId.id,
			setId.subId,
			static_cast<uint32_t>(piece)
		);
	}

	ArmorSeriesIDMap ArmourDataManager::getArmorSeriesData() {
		// Hunter Armors
		ArmorSeriesIDMap hunterArmors = getHunterArmorData();
		ArmorSeriesIDMap innerArmors = getInnersArmorData();

		// coalesce
		hunterArmors.insert(innerArmors.begin(), innerArmors.end());

		// Post process names
		postProcessArmorSeriesData(hunterArmors);

		return hunterArmors;
	}

	ArmorSeriesIDMap ArmourDataManager::getHunterArmorData() {
		ArmorSeriesIDMap map{};

		// Hunter Armour Sets
		static reframework::API::TypeDefinition* td_ArmorSeries = reframework::API::get()->tdb()->find_type("app.ArmorDef.SERIES");
		bool gotNumSeries = false;
		const size_t numSeries = REEnum(td_ArmorSeries, "MAX", gotNumSeries);
		if (!gotNumSeries) {
			DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_ERROR, "Failed to get number of Armor Series definitions!");
			return map;
		}

		size_t cappedNumSeries = std::min<size_t>(ARMOUR_DATA_FETCH_CAP, numSeries);
		DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_INFO, "Attempting to fetch {} armor series", cappedNumSeries);

		for (size_t i = 1; i < cappedNumSeries; i++) {
			std::string armorSeriesName = REInvokeGuidStatic("app.ArmorDef", "Name(app.ArmorDef.SERIES)", { (void*)i }, LocalizationLanguage::English);
			if (armorSeriesName == "-") continue;

			ArmorSeriesDisplayRank ranks = getArmorSeriesDisplayRank(armorSeriesName);

			int rawVariety = REInvokeStatic<int>("app.ArmorDef", "ModelVariety(app.ArmorDef.SERIES)", { (void*)i }, InvokeReturnType::DWORD);
			ArmorSeriesModelVariety variety = ArmorSeriesModelVariety::INVALID;
			switch (rawVariety) {
			case 0: variety = ArmorSeriesModelVariety::BOTH;   break;
			case 1: variety = ArmorSeriesModelVariety::MALE;   break;
			case 2: variety = ArmorSeriesModelVariety::FEMALE; break;
			}

			uint32_t modId = REInvokeStatic<uint32_t>("app.ArmorDef", "ModId(app.ArmorDef.SERIES)", { (void*)i }, InvokeReturnType::DWORD);
			auto handleInsertVariantMapping = [&](ArmorSeriesModelVariety flag, const char* subIdFuncSignature, bool isFemale) {
				if (!(variety & flag)) return;

				uint32_t modSubId = REInvokeStatic<uint32_t>("app.ArmorDef", subIdFuncSignature, { (void*)i }, InvokeReturnType::DWORD);
				ArmorSetID setId{ modId, modSubId };

				std::string seriesStem = getArmorSeriesNameStem(armorSeriesName);

				ArmourPieceFlags residentPieces = getResidentArmourPieces(i);

				//DEBUG_STACK.fpush<LOG_TAG>("Armor Series {} has resident pieces: {}{}{}{}{}",
				//	armorSeriesName,
				//	residentPieces & ArmourPieceFlagBits::APF_HELM ? "H" : "_",
				//	residentPieces & ArmourPieceFlagBits::APF_BODY ? "B" : "_",
				//	residentPieces & ArmourPieceFlagBits::APF_ARMS ? "A" : "_",
				//	residentPieces & ArmourPieceFlagBits::APF_COIL ? "C" : "_",
				//	residentPieces & ArmourPieceFlagBits::APF_LEGS ? "L" : "_");

				// Try insert, if already exists then update ranks
				auto [it, inserted] = map.try_emplace(
					setId,
					ArmorSeriesData{ seriesStem, isFemale, ranks, residentPieces }
				);

				if (!inserted) {
					it->second.ranks |= ranks;
				}
			};

			handleInsertVariantMapping(variety, "ModSubMaleId(app.ArmorDef.SERIES)", false);
			handleInsertVariantMapping(variety, "ModSubFemaleId(app.ArmorDef.SERIES)", true);

			DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_SUCCESS, "Fetched Armor Series Data for Idx {}: {}", i, armorSeriesName);
		}

		return map;
	}

	ArmorSeriesIDMap ArmourDataManager::getInnersArmorData() {
		ArmorSeriesIDMap map{};

		// Inner Sets
		static reframework::API::TypeDefinition* td_InnerStyle = reframework::API::get()->tdb()->find_type("app.characteredit.Definition.INNER_STYLE");
		bool gotNumInners = false;
		size_t numInners = REEnum(td_InnerStyle, "MAX", gotNumInners);
		if (!gotNumInners) {
			DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_ERROR, "Failed to get number of Inner Armour definitions!");
			return map;
		}
		DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_INFO, "Attempting to fetch {} inners", numInners);

		for (size_t i = 0; i < numInners; i++) {
			std::string innerRawName = REInvokeGuidStatic("app.ArmorUtil", "getInnerStyleName(app.characteredit.Definition.INNER_STYLE)", { (void*)i }, LocalizationLanguage::English);
			if (innerRawName == "-") continue;

			DEBUG_STACK.fpush<LOG_TAG>("Attempting to fetch inner Data for Idx {}: {}", i, innerRawName);

			// trim off ending <ICON_EQUIP_TYPE1> or <ICON_EQUIP_TYPE2> for gender
			constexpr std::string_view MALE_ICON   = "<ICON EQUIP_TYPE1>";
			constexpr std::string_view FEMALE_ICON = "<ICON EQUIP_TYPE2>";
			std::string innerName = innerRawName;
			bool female = false;
			if (innerName.ends_with(MALE_ICON)) {
				innerName.resize(innerName.size() - MALE_ICON.size());
			}
			else if (innerName.ends_with(FEMALE_ICON)) {
				innerName.resize(innerName.size() - FEMALE_ICON.size());
				female = true;
			}

			// ID and ranks
			ArmorSetID innerID = REInvokeStatic<ArmorSetID>("app.ArmorUtil", "getArmorSetIDFromInnerStyle(app.characteredit.Definition.INNER_STYLE)", { (void*)i }, InvokeReturnType::WORD);
			ArmorSeriesDisplayRank ranks = getArmorSeriesDisplayRank(innerName);
			std::string innerStem = getArmorSeriesNameStem(innerName);

			map.emplace(innerID, ArmorSeriesData{ innerStem, female, ranks, ArmourPieceFlagBits::APF_ALL ^ ArmourPieceFlagBits::APF_SET}); // Always have every piece

			DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_SUCCESS, "Fetched Inner Armour Set Idx {}: {}", i, innerName);
		}

		return map;
	}

	NpcPrefabToArmorSetMap ArmourDataManager::getNpcArmorData() {
		NpcPrefabToArmorSetMap map{};

		DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_INFO, "Attempting to fetch NPC Armor Data...");

		REApi::ManagedObject* cNpcCatalogHolder = REInvokePtr<REApi::ManagedObject>(npcManager.get(), "get_Catalog()", {});
		if (cNpcCatalogHolder == nullptr) {
			DEBUG_STACK.push("Failed to get NPC Catalog from NpcManager!", DebugStack::Color::COL_ERROR);
			return {};
		}

		// pass this twice to get an accurate count of how many NPCs were ignored
		size_t ignoredCnt = getNpcArmorDataDefaultSelectors(cNpcCatalogHolder, map, false);
		ignoredCnt        = getNpcArmorDataDefaultSelectors(cNpcCatalogHolder, map, true);

		DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_INFO, "Attempting to fetch unqiue NPC Armor Data...");

		getNpcArmorDataUniqueSelectors(cNpcCatalogHolder, map);

		DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_SUCCESS, "Successfully aliased {} NPC prefabs to armor sets!", map.size());
		if (ignoredCnt > 0) {
			DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_WARNING, "Ignored {} NPC prefabs which had no aliases defined.", ignoredCnt);
		}

		//app.user_data.NpcHunterEquipData.cArmorData

		return map;
	}

	size_t ArmourDataManager::getNpcArmorDataDefaultSelectors(REApi::ManagedObject* cNpcCatalogHolder, NpcPrefabToArmorSetMap& map, bool verbose) {
		size_t ignoredCount = 0;

		// 1. DEFAULT Selectors NpcVisualSettings
		for (size_t i = 0; i < NPC_ARMOR_PREFAB_FETCH_CAP; i++) {
			REApi::ManagedObject* NpcResidentPackage = REInvokePtr<REApi::ManagedObject>(cNpcCatalogHolder, "getResidentData(System.Int32)", { (void*)i });
			if (NpcResidentPackage == nullptr) continue;

			// Get name of NPC from System.Enum.GetName using index i
			static REApi::ManagedObject* td_NpcDefID = REApi::get()->typeof("app.NpcDef.ID");
			REApi::ManagedObject* boxedEnumValue = REInvokeStaticPtr<REApi::ManagedObject>("System.Enum", "InternalBoxEnum(System.RuntimeType, System.Int64)", { (void*)td_NpcDefID, (void*)i });
			std::string npcStrID = REInvokeStaticStr("System.Enum", "GetName(System.Type, System.Object)", { (void*)td_NpcDefID, (void*)boxedEnumValue });
			if (NpcDataManager::isPartnerNpcID(npcStrID)) continue; // Handle these in the unique selector pass as bits here aren't reliable.

			std::string npcName = NpcDataManager::get().getNpcNameFromID(i);

			REApi::ManagedObject* NpcVisualBase = REFieldPtr<REApi::ManagedObject>(NpcResidentPackage, "_VisualSetting");
			if (NpcVisualBase == nullptr) continue;

			static REApi::TypeDefinition* td_NpcVisualSetting = REApi::get()->tdb()->find_type("app.user_data.NpcVisualSetting");
			static REApi::TypeDefinition* td_NpcVisualSelector = REApi::get()->tdb()->find_type("app.user_data.NpcVisualSelector");
			REApi::TypeDefinition* td_visualSetting = NpcVisualBase->get_type_definition();
			if (td_visualSetting == nullptr) continue;

			// No Variants
			if (td_visualSetting == td_NpcVisualSetting) {
				REApi::ManagedObject* NpcVisualSetting = REInvokePtr<REApi::ManagedObject>(NpcResidentPackage, "get_VisualSetting()", {});
				std::string prefabPath = getPrefabFromVisualSetting(NpcVisualSetting);
				if (prefabPath.empty()) continue;

				size_t species = REInvoke<size_t>(NpcVisualSetting, "get_Species()", {}, InvokeReturnType::DWORD);
				if (species > 1) continue; // non human npc

				size_t gender = REInvoke<size_t>(NpcVisualSetting, "get_Gender()", {}, InvokeReturnType::DWORD);

				bool added = addPrefabToArmorSetMap(map, npcName, npcStrID, prefabPath, 0, gender == 1, verbose);
				if (!added) ignoredCount++;
			}
			// Variants
			else if (td_visualSetting == td_NpcVisualSelector) {
				REApi::ManagedObject* selector = REFieldPtr<REApi::ManagedObject>(NpcVisualBase, "_Selector");
				if (selector == nullptr) continue;

				const REApi::TypeDefinition* td_selector = selector->get_type_definition();
				const std::vector<REApi::Field*> selector_fields = td_selector->get_fields();

				for (size_t variantIdx = 0; variantIdx < selector_fields.size(); variantIdx++) {
					auto& field = selector_fields[variantIdx];

					if (field == nullptr) continue;
					if (field->get_type() != td_NpcVisualSetting) continue;

					REApi::ManagedObject* NpcVisualSetting = REFieldPtr<REApi::ManagedObject>(selector, field->get_name());
					std::string prefabPath = getPrefabFromVisualSetting(NpcVisualSetting);
					if (prefabPath.empty()) continue;

					size_t species = REInvoke<size_t>(NpcVisualSetting, "get_Species()", {}, InvokeReturnType::DWORD);
					if (species > 1) continue; // non human npc

					size_t gender = REInvoke<size_t>(NpcVisualSetting, "get_Gender()", {}, InvokeReturnType::DWORD);

					bool added = addPrefabToArmorSetMap(map, npcName, npcStrID, prefabPath, variantIdx, gender == 1, verbose);
					if (!added) ignoredCount++;
				}
			}

		}

		// Do a quick pass to filter out any sets we couldn't resolve
		std::unordered_set<std::string> mappingsToRemove{};
		for (const auto& [pth, data] : map) {
			if (data.name.empty() || (!data.femaleCanUse && !data.maleCanUse)) mappingsToRemove.insert(pth);
		}
		for (const std::string& pth : mappingsToRemove) {
			map.erase(pth);
		}

		return ignoredCount;
	}

	void ArmourDataManager::getNpcArmorDataUniqueSelectors(REApi::ManagedObject* cNpcCatalogHolder, NpcPrefabToArmorSetMap& map) {
		// These actually mirror the UNIQUE_VISUAL_Fixed IDs enum names, but just hardcode for now.
		//  Note: app.NpcDef.UNIQUE_VISUAL_Fixed mappings:
		//   2: Alma
		//   3: Erik
		//   4: Gemma
		// Update Note: If any other partners are added, you'll have to specifying their mappings here

		std::unordered_map<size_t, std::string> uniqueVisualIdxToPartnerID{
			{ 2, "NPC102_00_001" },
			{ 3, "NPC101_00_002" },
			{ 4, "NPC102_00_010" }
		};

		for (const auto& [idx, partnerStrID] : uniqueVisualIdxToPartnerID) {
			partnerIdToCostumePrefabMap[idx] = {};

			for (size_t variantIdx = 0; variantIdx < NPC_UNIQUE_PREFAB_VARIANTS_FETCH_CAP; variantIdx++) {
				REApi::ManagedObject* NpcVisualSetting = REInvokePtr<REApi::ManagedObject>(cNpcCatalogHolder, "getCustomVari(app.NpcDef.UNIQUE_VISUAL_Fixed, System.Int32)", { (void*)idx, (void*)variantIdx });
				std::string prefabPath = getPrefabFromVisualSetting(NpcVisualSetting);
				if (prefabPath.empty()) continue;

				size_t gender = REInvoke<size_t>(NpcVisualSetting, "get_Gender()", {}, InvokeReturnType::DWORD);

				addPrefabToArmorSetMap(map, partnerStrID, partnerStrID, prefabPath, variantIdx, gender == 1, true);
				partnerIdToCostumePrefabMap[idx].emplace(variantIdx, prefabPath);
			}
		}
	}

	bool ArmourDataManager::addPrefabToArmorSetMap(
		NpcPrefabToArmorSetMap& map,
		const std::string& npcName,
		const std::string& npcStrID,
		const std::string& prefabPath,
		size_t variant,
		bool female,
		bool verbose
	) {
		if (prefabPath.empty()) return false; // No prefab to map

		// A subset of npcs will be chosen to generate mappings from.
		std::string prefabAlias = NpcPrefabAliasMappings::getPrefabAlias(npcStrID, variant);
		// If we don't find a mapping, no issue, just TRY and find a matching prefab later cataloged by a seed NPC

		auto it = map.find(prefabPath);
		if (it != map.end()) {
			// Resolve Collision - Another NPC already mapped it, so just potentially update the gender that can use it, so long as the name matches.
			if (!prefabAlias.empty() && !it->second.name.empty() && prefabAlias != it->second.name) {
				if (verbose) DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_WARNING, "Prefab collision detected when mapping alias for NPC ({}, {}, [{}]): Old: {} | New: {}. This NPC's prefab alias will be ignored.", npcStrID, npcName, female ? "F" : "M", it->second.name, prefabAlias);
				return false;
			}
			else {
				// update the gender
				if (!prefabAlias.empty()) it->second.name = prefabAlias;
				it->second.femaleCanUse |= female;
				it->second.maleCanUse |= !female;
			}
		}
		else {
			// Add new entry
			NpcPrefabData newData{ prefabAlias, female, !female };
			map.emplace(prefabPath, newData);
		}

		if (verbose) {
			if (map.at(prefabPath).name.empty()) {
				DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_WARNING, "Pushed an empty NPC Prefab Mapping: ({}, {} [{}]) {} [{}] - Variant {}", npcStrID, npcName, female ? "F" : "M", map.at(prefabPath).name, prefabPath, variant);
			}
			else {
				DEBUG_STACK.fpush<LOG_TAG>(DebugStack::Color::COL_SUCCESS, "Loaded NPC Prefab Mapping: ({}, {} [{}]) {} [{}] - Variant {}", npcStrID, npcName, female ? "F" : "M", map.at(prefabPath).name, prefabPath, variant);
			}
		}

		return !map.at(prefabPath).name.empty();
	}

	ArmorSeriesDisplayRank ArmourDataManager::getArmorSeriesDisplayRank(const std::string& fullName) {
		constexpr const char* alphaChrU8 = "\xCE\xB1"; // α
		constexpr const char* betaChrU8  = "\xCE\xB2"; // β
		constexpr const char* gammaChrU8 = "\xCE\xB3"; // γ
		ArmorSeriesDisplayRank ranks = ArmorSeriesDisplayRankFlags::RANK_NONE;
		if (fullName.ends_with(alphaChrU8))      ranks |= ArmorSeriesDisplayRankFlags::RANK_ALPHA;
		else if (fullName.ends_with(betaChrU8))  ranks |= ArmorSeriesDisplayRankFlags::RANK_BETA;
		else if (fullName.ends_with(gammaChrU8)) ranks |= ArmorSeriesDisplayRankFlags::RANK_GAMMA;

		return ranks;
	}

	std::string ArmourDataManager::getArmorSeriesNameStem(const std::string& fullName) {
		// Remove any trailing α/β/γ symbols
		constexpr const char* alphaChrU8 = "\xCE\xB1"; // α
		constexpr const char* betaChrU8  = "\xCE\xB2"; // β
		constexpr const char* gammaChrU8 = "\xCE\xB3"; // γ

		// Note: these characters are 2 bytes, but also trim an extra byte for trailing space.
		if (fullName.ends_with(alphaChrU8)) {
			return fullName.substr(0, fullName.size() - 3);
		}
		else if (fullName.ends_with(betaChrU8)) {
			return fullName.substr(0, fullName.size() - 3);
		}
		else if (fullName.ends_with(gammaChrU8)) {
			return fullName.substr(0, fullName.size() - 3);
		}
		return fullName;
	}

	void ArmourDataManager::postProcessArmorSeriesData(ArmorSeriesIDMap& map) {
		// Update names to include correct display ranks.
		// I.e. 
		// RANK_NONE  = Armor Set
		// RANK_ALPHA = Armor Set 0
		// RANK_ALPHA | RANK_BETA = Armor Set 0/1
		// RANK_ALPHA | RANK_BETA | RANK_GAMMA = Armor Set 0/1/2

		for (auto& [id, data] : map) {
			std::string rankSuffix = "";
			if (data.ranks == ArmorSeriesDisplayRankFlags::RANK_NONE) {
				// No suffix
				continue;
			}
			else {
				std::vector<std::string> rankParts;
				if (data.ranks & ArmorSeriesDisplayRankFlags::RANK_ALPHA) rankParts.push_back("0");
				if (data.ranks & ArmorSeriesDisplayRankFlags::RANK_BETA)  rankParts.push_back("1");
				if (data.ranks & ArmorSeriesDisplayRankFlags::RANK_GAMMA) rankParts.push_back("2");
				rankSuffix = " " + std::accumulate(std::next(rankParts.begin()), rankParts.end(), rankParts[0],
					[](std::string a, std::string b) { return a + "/" + b; });
			}
			data.name = data.name + rankSuffix;
		}

	}

	std::string ArmourDataManager::getPrefabFromVisualSetting(REApi::ManagedObject* visualSetting) {
		if (visualSetting == nullptr) return "";

		REApi::ManagedObject* cNpcBaseModelData = REInvokePtr<REApi::ManagedObject>(visualSetting, "get_ModelData()", {});
		if (cNpcBaseModelData == nullptr) return "";

		REApi::ManagedObject* cBaseModelInfo = REInvokePtr<REApi::ManagedObject>(cNpcBaseModelData, "get_ModelInfo()", {});
		if (cBaseModelInfo == nullptr) return "";

		REApi::ManagedObject* prefab = REInvokePtr<REApi::ManagedObject>(cBaseModelInfo, "get_Prefab()", {});
		if (prefab == nullptr) return "";

		std::string prefabPath = REInvokeStr(prefab, "get_Path()", {});
		return prefabPath;
	}

}