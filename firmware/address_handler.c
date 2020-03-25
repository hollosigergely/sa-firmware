#include "address_handler.h"
#include <nrf.h>
#include <stdio.h>

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
			return 0xA001;
		case 6048785804469331297U:
			return 0xA003;
		case 13508677794463904490U:
			return 0xA000;
		case 3863683410104583303U:
			return 0x1000;
		case 14500113281835358815U:
			return 0xA002;
		case 5795675720523340107U:
			return 0xA005;
		case 10210119633631049802U:
			return 0xA004;

        case 822526009735700523U:
            return 0xB001;
        case 8490415443706040570U:
            return 0xB002;
        case 4233353996318710681U:
            return 0x2000;
		default:
			return 0xFFFF;
	}
}

uint16_t addr_handler_get_virtual_addr() {
	uint16_t addr = addr_handler_get();

	switch(addr)
	{
	case 0xA000:
		return 0;
	case 0xA001:
		return 1;
	case 0xA002:
		return 2;
	case 0xA003:
		return 3;
	case 0xA004:
		return 4;
	case 0xA005:
		return 5;
	case 0x1000:
		return 0;

    case 0xB001:
        return 0;
    case 0xB002:
        return 1;
    case 0x2000:
        return 0;

	case 0xFFFF:
	default:
		return 0xFFFF;
	}
}

bool addr_handler_is_anchor()
{
	uint16_t addr = addr_handler_get();
	switch(addr)
	{
	case 0xA000:
		return true;
	case 0xA001:
		return true;
	case 0xA002:
		return true;
	case 0xA003:
		return true;
	case 0xA004:
		return true;
	case 0xA005:
		return true;
	case 0x1000:
		return false;

    case 0xB001:
        return true;
    case 0xB002:
        return true;
    case 0x2000:
        return false;
	}
}

void addr_handler_get_device_name(char* device_name)
{
    if(addr_handler_is_anchor())
    {
        sprintf(device_name, "Anchor 0x%04X", addr_handler_get());
    }
    else
    {
        sprintf(device_name, "Tag 0x%04X", addr_handler_get());
    }
}


uint16_t addr_handler_get_group_id()
{
    return 0xBABA;
}
