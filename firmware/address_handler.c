#include "address_handler.h"
#include "log.h"
#include <nrf.h>
#include <stdio.h>
#include <inttypes.h>

#define TAG "addr"

typedef struct  {
    uint64_t    mac_addr;
    uint16_t    virt_addr;
    bool        is_anchor;
} device_addr_t;


static device_addr_t m_device_db[] = {
// Tags
    { 0xD90731EA504E, 0x0000, false },

// Anchors
    { 0xFDE6286BE9AB, 0x0000, true },
    { 0x5C676BE0E853, 0x0001, true },
    { 0xCF458A24912A, 0x0002, true },
    { 0x4FE21045C08E, 0x0003, true },
    { 0x0DF4EC56D725, 0x0004, true },
    { 0x61DB9F4C6157, 0x0005, true },

};
#define DEVICE_DB_LENGTH    (sizeof(m_device_db)/sizeof(m_device_db[0]))

static uint64_t m_mac_address;
static uint16_t m_virtual_address;
static bool     m_is_anchor;
static char     m_device_name[26];

void addr_handler_init() {
    m_mac_address = *((uint64_t*)&NRF_FICR->DEVICEADDR[0]);
    m_mac_address &= 0x0000ffffffffffff;
    m_mac_address |= 0x0000c00000000000;

    LOGI(TAG,"MAC addr: 0x%" PRIX64 "\n", m_mac_address);

    for(size_t i = 0; i < DEVICE_DB_LENGTH; i++) {
        if((m_device_db[i].mac_addr | 0x0000c00000000000) == m_mac_address)
        {
            m_virtual_address = m_device_db[i].virt_addr;
            m_is_anchor = m_device_db[i].is_anchor;

            if(m_is_anchor)
            {
                sprintf(m_device_name, "Anchor 0x%04X", addr_handler_get_virtual_addr());
            }
            else
            {
                sprintf(m_device_name, "Tag 0x%04X", addr_handler_get_virtual_addr());
            }

            LOGI(TAG, "device: %s\n", m_device_name);
            LOGI(TAG, "group: 0x%04X\n", addr_handler_get_group_id());

            return;
        }
    }

    ERROR(TAG,"No address specified.\n");
}

uint64_t addr_handler_get_mac_addr() {
    return m_mac_address;
}

uint16_t addr_handler_get_virtual_addr() {
    return m_virtual_address;
}

bool addr_handler_is_anchor()
{
    return m_is_anchor;
}

char *addr_handler_get_device_name()
{
    return m_device_name;
}


uint16_t addr_handler_get_group_id()
{
    return 0xBABA;
}


