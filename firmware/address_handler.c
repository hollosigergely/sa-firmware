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
			return 0xB0C1;
		case 10210119633631049802U:
			return 0xBABA;
		case 2518662390142021780U:
			return 0xDEAD;
		case 424297649018654323U:
			return 0xB0C1;
		case 3863683410104583303U:
			return 0xCAFE;
		case 4484055259050763026U:
			return 0xB0D1;
		case 14177588937323015699U:
			return 0xF00D;
		case 971774005844853828U:
			return 0xFACE;
		case 6048785804469331297U:
			return 0xC001;
		case 14500113281835358815U:
			return 0xB0B1;
		case 13508677794463904490U:
			return 0xC0DE;
		case 5795675720523340107U:
			return 0xFEED;

		case 14987045165802062477U:
			return 0xB0C1;
		case 11990617330264141128U:
			return 0xBABA;
		case 14182497569501676789U:
			return 0xDEAD;
		case 17089474243666811465U:
			return 0xBEEF;
		case 4010061173696466314U:
			return 0xCAFE;
		case 17617268231626340086U:
			return 0xB0D1;
		case 1360040707141322333U:
			return 0xF00D;
		case 300660264184283174U:
			return 0xFACE;
		case 15771100700842021796U:
			return 0xC001;
		case 10117951931529112199U:
			return 0xB0B1;
		case 15659757225032033991U:
			return 0xC0DE;
		case 4953354990034652202U:
			return 0xFEED;
		default:
			return 0;
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

