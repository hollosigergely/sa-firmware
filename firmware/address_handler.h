#ifndef ADDRESS_HANDLER_H
#define ADDRESS_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

uint64_t addr_handler_get_chip_id();
uint16_t addr_handler_get();
uint16_t addr_handler_get_virtual_addr();
bool addr_handler_is_anchor();
void addr_handler_get_device_name(char* device_name);

#endif // ADDRESS_HANDLER_H
