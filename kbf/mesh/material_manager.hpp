#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/data/armour/armour_info.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/mesh/materials/mesh_material.hpp>

#include <reframework/API.hpp>

using REApi = reframework::API;

namespace kbf {

	class MaterialManager {
	public:
		MaterialManager(
			KBFDataManager& datamanager,
			ArmourInfo armour,
			REApi::ManagedObject* baseTransform,
			REApi::ManagedObject* helmTransform,
			REApi::ManagedObject* bodyTransform,
			REApi::ManagedObject* armsTransform,
			REApi::ManagedObject* coilTransform,
			REApi::ManagedObject* legsTransform,
			bool female);

		bool applyPreset(const Preset* preset, ArmourPiece piece);
		bool loadMaterials();

		bool isInitialized() const { return initialized; }

	private:
		using QuickOverrideMatMatchLUT = std::unordered_map<std::string, std::vector<const MeshMaterial*>>;
		
		void applyQuickOverrides(
			const Preset* preset, 
			REApi::ManagedObject* mesh,
			QuickOverrideMatMatchLUT& matches);

		bool getMesh(REApi::ManagedObject* transform, REApi::ManagedObject** out) const;
		std::unordered_map<std::string, MeshMaterial> getMaterials(
			REApi::ManagedObject* mesh, 
			QuickOverrideMatMatchLUT* quickOverrideMatchesOut
		) const;
		QuickOverrideMatMatchLUT getQuickOverrideMatches(const MeshMaterial& mat) const;

		KBFDataManager& dataManager;
		ArmourInfo armourInfo;
		bool female;
		bool initialized = false;

		std::unordered_map<std::string, MeshMaterial> helmMaterials{};
		std::unordered_map<std::string, MeshMaterial> bodyMaterials{};
		std::unordered_map<std::string, MeshMaterial> armsMaterials{};
		std::unordered_map<std::string, MeshMaterial> coilMaterials{};
		std::unordered_map<std::string, MeshMaterial> legsMaterials{};

		QuickOverrideMatMatchLUT helmQuickOverrideMatches{};
		QuickOverrideMatMatchLUT bodyQuickOverrideMatches{};
		QuickOverrideMatMatchLUT armsQuickOverrideMatches{};
		QuickOverrideMatMatchLUT coilQuickOverrideMatches{};
		QuickOverrideMatMatchLUT legsQuickOverrideMatches{};

		REApi::ManagedObject* baseTransform = nullptr;
		REApi::ManagedObject* helmTransform = nullptr;
		REApi::ManagedObject* bodyTransform = nullptr;
		REApi::ManagedObject* armsTransform = nullptr;
		REApi::ManagedObject* coilTransform = nullptr;
		REApi::ManagedObject* legsTransform = nullptr;

		REApi::ManagedObject* helmMesh = nullptr;
		REApi::ManagedObject* bodyMesh = nullptr;
		REApi::ManagedObject* armsMesh = nullptr;
		REApi::ManagedObject* coilMesh = nullptr;
		REApi::ManagedObject* legsMesh = nullptr;
	};

}