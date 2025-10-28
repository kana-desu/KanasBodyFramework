#pragma once

#include <kbf/data/preset/preset.hpp>
#include <kbf/data/preset/preset_group.hpp>

namespace kbf {

	class  EditableObject {
	public:

        ~EditableObject() {
            clear();
        }

        EditableObject()
            : type(ObjectType::NONE) {
            ptrBefore.preset = nullptr;
            ptrAfter.preset = nullptr;
        }

        EditableObject(const EditableObject& other)
            : type(other.type) {
            ptrBefore = other.ptrBefore;

            switch (type) {
            case ObjectType::PRESET:
                ptrAfter.preset = other.ptrAfter.preset ? new Preset(*other.ptrAfter.preset) : nullptr;
                break;
            case ObjectType::PRESET_GROUP:
                ptrAfter.presetGroup = other.ptrAfter.presetGroup ? new PresetGroup(*other.ptrAfter.presetGroup) : nullptr;
                break;
            case ObjectType::NONE:
                ptrAfter.preset = nullptr;
                break;
            }
        }

        EditableObject& operator=(const EditableObject& other) {
            if (this == &other)
                return *this;

            clear();

            type = other.type;
            ptrBefore = other.ptrBefore;

            switch (type) {
            case ObjectType::PRESET:
                ptrAfter.preset = other.ptrAfter.preset ? new Preset(*other.ptrAfter.preset) : nullptr;
                break;
            case ObjectType::PRESET_GROUP:
                ptrAfter.presetGroup = other.ptrAfter.presetGroup ? new PresetGroup(*other.ptrAfter.presetGroup) : nullptr;
                break;
            case ObjectType::NONE:
                ptrAfter.preset = nullptr;
                break;
            }

            return *this;
        }

		enum ObjectType {
			NONE,
			PRESET,
			PRESET_GROUP
		} type;

		union PresetOrGroupPtr {
			PresetGroup* presetGroup;
			Preset* preset;

			PresetOrGroupPtr() : preset(nullptr) {}
		};

		PresetOrGroupPtr ptrBefore;
		PresetOrGroupPtr ptrAfter;

		bool notSet() const { return type == ObjectType::NONE; }

		void setNone() { clear(); type = ObjectType::NONE; }

        void setPresetCurrent(const Preset* preset) {
            if (preset == nullptr) return;
            if (type != ObjectType::PRESET) return;

            clear();
            ptrAfter.preset = new Preset(*preset);
        }

		void setPreset(Preset* preset) { 
			if (preset == nullptr) return setNone();
			clear();
			type = ObjectType::PRESET;
			ptrBefore.preset = preset;
			ptrAfter.preset = new Preset(*preset);
		}

        void revertPreset() {
            if (type != ObjectType::PRESET) return;
            clear();
            ptrAfter.preset = new Preset(*ptrBefore.preset);
        }

        void setPresetGroupCurrent(const PresetGroup* presetGroup) {
            if (presetGroup == nullptr) return;
            if (type != ObjectType::PRESET_GROUP) return;

            clear();
            ptrAfter.presetGroup = new PresetGroup(*presetGroup);
        }

		void setPresetGroup(PresetGroup* presetGroup) { 
			if (presetGroup == nullptr) return setNone();
			clear();
			type = ObjectType::PRESET_GROUP;
			ptrBefore.presetGroup = presetGroup;
			ptrAfter.presetGroup  = new PresetGroup(*presetGroup);
		}

        void revertPresetGroup() {
            if (type != ObjectType::PRESET_GROUP) return;
            clear();
            ptrAfter.presetGroup = new PresetGroup(*ptrBefore.presetGroup);
        }

	private:
		void clear() {
			if (type == ObjectType::PRESET && ptrAfter.preset != nullptr)
				delete ptrAfter.preset;
			else if (type == ObjectType::PRESET_GROUP && ptrAfter.presetGroup != nullptr)
				delete ptrAfter.presetGroup;

			ptrAfter.preset = nullptr;
		}

	};

}