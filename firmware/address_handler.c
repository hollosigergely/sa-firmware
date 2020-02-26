#include "address_handler.h"
#include <nrf.h>

uint64_t addr_handler_get_chip_id() {
	uint64_t deviceID = NRF_FICR->DEVICEID[1];
	deviceID = (deviceID << 32) & 0xFFFFFFFF00000000;
	deviceID |= NRF_FICR->DEVICEID[0] & 0xFFFFFFFF;
	return deviceID;
}

uint16_t addr_handler_get()
{
	uint64_t deviceID = addr_handler_get_chip_id();

	switch(deviceID)
	{
		case 4529750270100757500U:
			return 0xBEEF;
		case 6048785804469331297U:
			return 0xC001;
		case 13508677794463904490U:
			return 0xC0DE;
		default:
			return 0xFFFF;
	}
}

uint16_t addr_handler_get_virtual_addr() {
	uint16_t addr = addr_handler_get();

	switch(addr)
	{
	case 0xC0DE:
		return 0;
	case 0xC001:
		return 1;
	case 0xBEEF:
		return 2;
	case 0xFFFF:
	default:
		return 0xFFFF;
	}
}

bool isAnchor(uint16_t address)
{
	switch(address)
	{
		case 0xDEAD:
			return true;
		case 0xB0D1:
			return true;
		case 0xF00D:
			return true;
		case 0xFACE:
			return true;
		case 0xC001:
			return true;
		case 0xB0B1:
			return true;
		case 0xC0DE:
			return true;
		case 0xCAFE:
			return true;

		case 0xB0C1:
			return false;
		case 0xBABA:
			return false;
		case 0xBEEF:
			return false;
		case 0xFEED:
			return false;
	default:
			return false;
	}
}

