#pragma once

#include <kbf/data/kbf_data_manager.hpp>
#include <kbf/data/armour/armour_info.hpp>
#include <kbf/data/preset/preset.hpp>
#include <kbf/data/mesh/mesh_part.hpp>

#include <reframework/API.hpp>

using REApi = reframework::API;

namespace kbf {

	class PartManager {
	public:
		PartManager(
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
		bool loadParts();

		bool isInitialized() const { return initialized; }

	private:
		//std::set<std::string> getPartNames(REApi::ManagedObject* jointArr) const;
		//void DEBUG_printPartNames(REApi::ManagedObject* jointArr, std::string message) const;
		bool getMesh(REApi::ManagedObject* transform, REApi::ManagedObject** out) const;
		void getParts(REApi::ManagedObject* mesh, std::vector<MeshPart>& out) const;

		KBFDataManager& dataManager;
		ArmourInfo armourInfo;
		bool female;
		bool initialized = false;

		std::vector<MeshPart> baseParts{};
		std::vector<MeshPart> helmParts{};
		std::vector<MeshPart> bodyParts{};
		std::vector<MeshPart> armsParts{};
		std::vector<MeshPart> coilParts{};
		std::vector<MeshPart> legsParts{};

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