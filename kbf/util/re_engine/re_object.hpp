#pragma once

#include <cstdint>

namespace {

	class REObject
	{
	public:
		char pad_WTF[8]; //0x0000
	}; //Size: 0x0008
	static_assert(sizeof(REObject) == 0x8);

	class REManagedObject : public REObject
	{
	public:
		uint32_t referenceCount; //0x0008
		int16_t N000071AE; //0x000C
		char pad_000E[2]; //0x000E
	}; //Size: 0x0010
	static_assert(sizeof(REManagedObject) == 0x10);

}