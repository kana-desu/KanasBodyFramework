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
		bool getMesh(REApi::ManagedObject* transform, REApi::ManagedObject** out) const;
		void getMaterials(REApi::ManagedObject* mesh, std::vector<MeshMaterial>& out) const;

		KBFDataManager& dataManager;
		ArmourInfo armourInfo;
		bool female;
		bool initialized = false;

		std::vector<MeshMaterial> helmMaterials{};
		std::vector<MeshMaterial> bodyMaterials{};
		std::vector<MeshMaterial> armsMaterials{};
		std::vector<MeshMaterial> coilMaterials{};
		std::vector<MeshMaterial> legsMaterials{};

		REApi::ManagedObject* baseTransform = nullptr;
		REApi::ManagedObject* helmTransform = nullptr;
		REApi::ManagedObject* bodyTransform = nullptr;
		REApi::ManagedObject* armsTransform = nullptr;
		REApi::ManagedObject* coilTransform = nullptr;
		REApi::ManagedObject* legsTransform = nullptr;

		REApi::ManagedObject* baseMesh = nullptr;
		REApi::ManagedObject* helmMesh = nullptr;
		REApi::ManagedObject* bodyMesh = nullptr;
		REApi::ManagedObject* armsMesh = nullptr;
		REApi::ManagedObject* coilMesh = nullptr;
		REApi::ManagedObject* legsMesh = nullptr;
	};

}