#pragma once

#include <kbf/data/mesh/parts/mesh_part.hpp>

namespace kbf {

	struct OverrideMeshPart {

		OverrideMeshPart() = default;
		OverrideMeshPart(MeshPart part, bool shown = false)
			: part{ part }, shown{ shown } {}

		bool operator==(const OverrideMeshPart& other) const {
			return part == other.part && shown == other.shown;
		}

		bool operator==(const MeshPart& other) const {
			return part == other;
		}

		bool operator<(const OverrideMeshPart& other) const {
			return part < other.part;
		}

		bool operator<(const MeshPart& other) const {
			return part < other;
		}

		MeshPart part;
		bool shown = false; // default to removed

	};

}