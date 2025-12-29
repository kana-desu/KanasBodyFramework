#pragma once

#include <kbf/data/armour/armour_piece.hpp>
#include <kbf/data/armour/armour_set.hpp>
#include <kbf/debug/debug_stack.hpp>
#include <kbf/util/string/cvt_utf16_utf8.hpp>
#include <kbf/data/file/kbf_file_upgrader.hpp>

#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>

#include <string>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <kbf/data/armour/armour_list.hpp>

#define CACHE_MANAGER_LOG_TAG "[CacheManager]"

namespace kbf {

	enum class CacheManagerType {
		BONES,
		PARTS,
		MATERIALS
	};

	template<typename CacheType, typename CacheIDType>
	class CacheManager {
	public:
		CacheManager(CacheManagerType type, const std::filesystem::path& cachesPath) 
			: type{ type }, cachesPath{ cachesPath } 
		{
			verifyDirectoryExists();
		}

		bool loadCaches() {
			verifyDirectoryExists();

			bool hasFailure = false;

			DEBUG_STACK.push(std::format("{} Loading caches from \"{}\"...", CACHE_MANAGER_LOG_TAG, cachesPath.string()), DebugStack::Color::COL_INFO);

			// Load all presets from the preset directory
			for (const auto& entry : std::filesystem::directory_iterator(cachesPath)) {
				if (entry.is_regular_file() && entry.path().extension() == ".json") {
					CacheType cache{};

					if (loadCache(entry.path(), &cache)) {
						caches.emplace(cache.armour, cache);
						//DEBUG_STACK.push(std::format("{} Loaded cache: {} ({}-{})",
						//	CACHE_MANAGER_LOG_TAG,
						//	partCache.armour.set.name,
						//	partCache.armour.characterFemale ? "F" : "M",
						//	partCache.armour.set.female ? "F" : "M"
						//), DebugStack::Color::SUCCESS);
					}
					else {
						hasFailure = true;
					}
				}
			}
			return hasFailure;
		}


		virtual void cache(const ArmourSetWithCharacterSex& armour, const std::vector<CacheIDType>& parts, ArmourPiece piece) = 0;

		const CacheType* getCache(const ArmourSetWithCharacterSex& armour) const {
			if (caches.find(armour) == caches.end()) return nullptr;
			return &caches.at(armour);
		}

		std::vector<ArmourSetWithCharacterSex> getCachedArmourSets() const {
			std::vector<ArmourSetWithCharacterSex> armourSets;
			for (const auto& [key, _] : caches) {
				armourSets.push_back(key);
			}
			return armourSets;
		}

	protected:
		const CacheManagerType type;
		const std::filesystem::path cachesPath;

		std::unordered_map<ArmourSetWithCharacterSex, CacheType> caches;

		void verifyDirectoryExists() const {
			if (!std::filesystem::exists(cachesPath)) {
				std::filesystem::create_directories(cachesPath);
			}
		}

		bool loadCache(const std::filesystem::path& path, CacheType* out) {
			assert(out != nullptr);

			DEBUG_STACK.push(std::format("{} Loading Cache @ \"{}\"...", CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_INFO);

			ArmourSetWithCharacterSex armour;

			if (!getCacheArmourSet(path.stem().string(), &armour)) {
				DEBUG_STACK.push(std::format("{} Cache @ {} has an invalid name. Skipping...", CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_WARNING);
				return false;
			}

			rapidjson::Document doc = loadCacheJson(path.string());
			if (!doc.IsObject() || doc.HasParseError()) return false;

			CacheType cache{};
			bool parsed = getCacheFromDocument(doc, armour, cache);

			if (!parsed) {
				DEBUG_STACK.push(std::format("{} Failed to parse cache \"{}\". One or more required values were missing. Please rectify or remove the file.", CACHE_MANAGER_LOG_TAG, path.string()), DebugStack::Color::COL_ERROR);
			}
			else {
				*out = cache;
			}

			return parsed;
		}

		virtual bool getCacheFromDocument(const rapidjson::Document& doc, ArmourSetWithCharacterSex armour, CacheType& out) const = 0;

		rapidjson::Document loadCacheJson(const std::string& path) const {
			bool exists = std::filesystem::exists(path);
			if (!exists) {
				DEBUG_STACK.push(std::format("{} Could not find json at {}. Please rectify or delete the file.", CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
			}

			std::string json = readJsonFile(path);

			rapidjson::Document config;
			config.Parse(json.c_str());

			if (!config.IsObject() || config.HasParseError()) {
				DEBUG_STACK.push(std::format("{} Failed to parse json at {}. Please rectify or delete the file.", CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
			}

			// Handle cache upgrade
			KbfFileType fileType = KbfFileType::UNKNOWN;
			switch (type) {
			case CacheManagerType::BONES:     fileType = KbfFileType::BONE_CACHE;     break;
			case CacheManagerType::PARTS:     fileType = KbfFileType::PART_CACHE;     break;
			case CacheManagerType::MATERIALS: fileType = KbfFileType::MATERIAL_CACHE; break;
			}

			static KbfFileUpgrader upgrader{};
			KbfFileUpgrader::UpgradeResult res = upgrader.upgradeFile(config, fileType);

			if (res == KbfFileUpgrader::UpgradeResult::FAILED) {
				DEBUG_STACK.push(std::format("{} Failed to upgrade json file at {}. Please rectify or delete the file.", CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::COL_ERROR);
			}

			if (res == KbfFileUpgrader::UpgradeResult::SUCCESS) {
				DEBUG_STACK.push(std::format("{} Upgraded json file at \"{}\" to the latest format.", CACHE_MANAGER_LOG_TAG, path), DebugStack::Color::COL_SUCCESS);

				// Before rewriting the file, make a backup of the existing one
				std::string backupPath = path + ".backup";
				writeJsonFile(backupPath, json);
				DEBUG_STACK.push(std::format("{} Created backup of the previous version at {}", CACHE_MANAGER_LOG_TAG, backupPath), DebugStack::Color::COL_INFO);

				// Write the upgraded file back to disk
				rapidjson::StringBuffer s;
				rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(s);
				config.Accept(writer);
				writeJsonFile(path, s.GetString());
			}

			return config;
		}
		
		// UNSAFE - Do not use directly. Call loadCacheJson instead.
		std::string readJsonFile(const std::string& path) const {
			std::ifstream file{ path, std::ios::ate };

			if (!file.is_open()) {
				const std::string error_pth_out{ path };
				DEBUG_STACK.push(std::format("{} Could not open json file for read at {}", CACHE_MANAGER_LOG_TAG, error_pth_out), DebugStack::Color::COL_ERROR);
				return "";
			}

			size_t fileSize = static_cast<size_t>(file.tellg());
			std::string buffer(fileSize, ' ');
			file.seekg(0);
			file.read(buffer.data(), fileSize);

			file.close();

			return buffer;
		}

		bool writeJsonFile(std::string path, const std::string& json) const {
			std::wstring wpath = cvt_utf8_to_utf16(path);

			// Open file in binary mode to avoid any encoding conversion
			std::ofstream file(wpath, std::ios::binary | std::ios::trunc);
			if (!file.is_open()) return false;

			// Write JSON content as-is (assumed UTF-8)
			file.write(json.data(), json.size());
			file.close();

			return true;
		}

		bool writeCache(const ArmourSetWithCharacterSex& armour) const {
			if (caches.find(armour) == caches.end()) {
				DEBUG_STACK.push(std::format("{} Tried to write cache for armour set {} ({}-{}), but no cache data exists.",
					CACHE_MANAGER_LOG_TAG,
					armour.set.name,
					armour.characterFemale ? "F" : "M",
					armour.set.female ? "F" : "M"
				), DebugStack::Color::COL_ERROR);
				return false;
			}

			return writeCacheJson(cachesPath / getCacheFilename(armour), caches.at(armour));
		}

		virtual bool writeCacheJson(const std::filesystem::path& path, const CacheType& out) const = 0;

		std::string getCacheFilename(const ArmourSetWithCharacterSex& armour) const {
			std::string characterSex = (armour.characterFemale ? "F" : "M");
			std::string sex = (armour.set.female ? "F" : "M");

			std::string armourNameSanitized = armour.set.name;
			// Replace any illegal filename characters in the armour set name with underscores
			std::replace(armourNameSanitized.begin(), armourNameSanitized.end(), '/', '_');
			// Replace dots with ampersands to not conflict with file extension
			std::replace(armourNameSanitized.begin(), armourNameSanitized.end(), '.', '&');

			return std::format("{}-{}~{}.json", characterSex, sex, armourNameSanitized);
		}

		bool getCacheArmourSet(const std::string& filename, ArmourSetWithCharacterSex* out) const {
			assert(out != nullptr);

			if (filename.size() < 4)
				return false;

			char characterSex = filename[0];
			if (characterSex != 'M' && characterSex != 'F')
				return false;

			char sex = filename[2];
			if (sex != 'M' && sex != 'F')
				return false;

			bool characterFemale = (characterSex == 'F');
			bool female = (sex == 'F');
			std::string armourName = filename.substr(4);
			armourName = armourName.substr(0, armourName.find_last_of('.')); // Remove json file extension

			// Restore illegal characters
			std::replace(armourName.begin(), armourName.end(), '_', '/');
			// Restore dots
			std::replace(armourName.begin(), armourName.end(), '&', '.');

			if (!ArmourList::isValidArmourSet(armourName, female))
				return false;

			*out = ArmourSetWithCharacterSex{ ArmourSet{ armourName, female }, characterFemale };
			return true;
		}
	};

}