#ifndef ADDRESS_HANDLER_H
#define ADDRESS_HANDLER_H

#include <stdbool.h>
#include <stdint.h>

uint64_t addr_handler_get_chip_id();
uint16_t addr_handler_get();
bool isAnchor(uint16_t address);


#endif // ADDRESS_HANDLER_H
