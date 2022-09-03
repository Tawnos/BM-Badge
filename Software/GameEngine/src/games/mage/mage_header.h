/*
This class contains the MageHeader class and all related subclasses
It is a structure used to hold the binary information in the ROM
in a more accessible way.
*/
#ifndef _MAGE_HEADER_H
#define _MAGE_HEADER_H

#include "mage_defines.h"
#include "EngineROM.h"

class MageHeader
{
private:
	uint32_t counts{ 0 };
	std::unique_ptr<uint32_t[]> offsets{std::make_unique<uint32_t[]>(1)};
	std::unique_ptr<uint32_t[]> lengths{std::make_unique<uint32_t[]>(1)};

public:
	MageHeader() = default;
	MageHeader(std::shared_ptr<EngineROM> ROM, uint32_t address);

	uint32_t count() const;
	uint32_t offset(uint32_t num) const;
	uint32_t length(uint32_t num) const;
	uint32_t size() const;
	bool valid() const;
}; //class MageHeader

#endif //_MAGE_HEADER_H
