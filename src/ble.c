#include "nrf.h"
#include "ble.h"
#include "main.h"
#include "compiler.h"

#include <string.h>
#include <stdbool.h>

#ifdef LOG
#include "timer.h"
#include "rtt/SEGGER_RTT.h"
#endif

uint8_t access_address[4] = {0xD6, 0xBE, 0x89, 0x8E};
uint8_t seed[3] = {0x55, 0x55, 0x55};


#ifdef NRF52820_XXAA
#define MAX_TX_POWER 0x8
#else
#define MAX_TX_POWER 0x4
#endif

/**@brief The maximum possible length in device discovery mode. */
#define DD_MAX_PAYLOAD_LENGTH         (31 + 6)


/**
 * @brief Short task definition
 * 
 * Ready Start => When radio is ready automatically start sending
 * End Disable => When radio has sent data disable it directly afterwards
 */
#define DEFAULT_RADIO_SHORTS                                             \
(                                                                        \
    (RADIO_SHORTS_READY_START_Enabled << RADIO_SHORTS_READY_START_Pos) | \
    (RADIO_SHORTS_END_DISABLE_Enabled << RADIO_SHORTS_END_DISABLE_Pos)   \
)


/**@brief The default CRC init polynominal. 
   @note Written in little endian but stored in big endian, because the BLE spec. prints
         is in little endian but the HW stores it in big endian. */
#define CRC_POLYNOMIAL_INIT_SETTINGS  ((0x5B << 0) | (0x06 << 8) | (0x00 << 16))


/**@brief This macro converts the given channel index to a freuency
          offset (i.e. offset from 2400 MHz).
  @param index - the channel index to be converted into frequency offset.
  
  @return The frequency offset for the given index. */
#define CHANNEL_IDX_TO_FREQ_OFFS(index) \
    (((index) == 37)?\
        (2)\
        :\
            (((index) == 38)?\
                (26)\
            :\
                (((index) == 39)?\
                    (80)\
                :\
                    ((/*((index) >= 0) &&*/ ((index) <= 10))?\
                        ((index)*2 + 4)\
                    :\
                        ((index)*2 + 6)))))

#define BD_ADDR_OFFS                (3)     /* BLE device address offest of the beacon advertising pdu. */

static void (*onDisableCB)();

RAM_CODE void RADIO_IRQHandler(void)
{
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> BLE: Interrupt\r\n", timer_get_seconds());
    #endif

    if (NRF_RADIO->EVENTS_DISABLED) 
    {
        NRF_RADIO->EVENTS_DISABLED = 0;

        // We need to check if the callback is the same since it can change
        // when the callback is fired due to send calling inside a callback
        void* before = onDisableCB;
        onDisableCB();
        if (onDisableCB == before) 
        {
            onDisableCB = NULL;
        }
    }
}

RAM_CODE void ble_send_on_channel(uint8_t channel_index, uint8_t * data, void (*cb)())
{
    onDisableCB = cb;

    // Set channel
    NRF_RADIO->FREQUENCY = CHANNEL_IDX_TO_FREQ_OFFS(channel_index);
    NRF_RADIO->DATAWHITEIV = channel_index;

    // Send data
    // We only start sending when radio is disabled
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> BLE: Sending data on channel: ", timer_get_seconds());
    SEGGER_RTT_printf(0, "%2.2u\r\n", channel_index);
    #endif

    NRF_RADIO->PACKETPTR = (uint32_t) &(data[0]);
    NRF_RADIO->EVENTS_DISABLED = 0;
    NRF_RADIO->TASKS_TXEN = 1;
}

RAM_CODE void ble_init(void) 
{
    NVIC_DisableIRQ(RADIO_IRQn);

    // Ensure that we are power reset
    NRF_RADIO->POWER = RADIO_POWER_POWER_Disabled << RADIO_POWER_POWER_Pos;
    NRF_RADIO->POWER = RADIO_POWER_POWER_Enabled << RADIO_POWER_POWER_Pos;

    // Put in short links
    NRF_RADIO->SHORTS = DEFAULT_RADIO_SHORTS;
    
    NRF_RADIO->PCNF0 =   (((1UL) << RADIO_PCNF0_S0LEN_Pos) & RADIO_PCNF0_S0LEN_Msk)
                       | (((2UL) << RADIO_PCNF0_S1LEN_Pos) & RADIO_PCNF0_S1LEN_Msk)
                       | (((6UL) << RADIO_PCNF0_LFLEN_Pos) & RADIO_PCNF0_LFLEN_Msk);

    NRF_RADIO->PCNF1 =   (((RADIO_PCNF1_ENDIAN_Little)        << RADIO_PCNF1_ENDIAN_Pos) & RADIO_PCNF1_ENDIAN_Msk)
                       | (((3UL)                              << RADIO_PCNF1_BALEN_Pos)  & RADIO_PCNF1_BALEN_Msk)
                       | (((0UL)                              << RADIO_PCNF1_STATLEN_Pos)& RADIO_PCNF1_STATLEN_Msk)
                       | ((((uint32_t)DD_MAX_PAYLOAD_LENGTH)  << RADIO_PCNF1_MAXLEN_Pos) & RADIO_PCNF1_MAXLEN_Msk)
                       | ((RADIO_PCNF1_WHITEEN_Enabled << RADIO_PCNF1_WHITEEN_Pos) & RADIO_PCNF1_WHITEEN_Msk);
    
    /* The CRC polynomial is fixed, and is set here. */
    /* The CRC initial value may change, and is set by */
    /* higher level modules as needed. */
    NRF_RADIO->CRCPOLY = (uint32_t)CRC_POLYNOMIAL_INIT_SETTINGS;
    NRF_RADIO->CRCCNF = (((RADIO_CRCCNF_SKIPADDR_Skip) << RADIO_CRCCNF_SKIPADDR_Pos) & RADIO_CRCCNF_SKIPADDR_Msk)
                      | (((RADIO_CRCCNF_LEN_Three)      << RADIO_CRCCNF_LEN_Pos)       & RADIO_CRCCNF_LEN_Msk);

    NRF_RADIO->RXADDRESSES  = ( (RADIO_RXADDRESSES_ADDR0_Enabled) << RADIO_RXADDRESSES_ADDR0_Pos);

    NRF_RADIO->MODE    = ((RADIO_MODE_MODE_Ble_1Mbit) << RADIO_MODE_MODE_Pos) & RADIO_MODE_MODE_Msk;

    NRF_RADIO->TIFS = 150; // Time in mircos between two packets

    // Our access adress
    NRF_RADIO->PREFIX0 = access_address[3];
    NRF_RADIO->BASE0   = ( (((uint32_t)access_address[2]) << 24) 
                         | (((uint32_t)access_address[1]) << 16)
                         | (((uint32_t)access_address[0]) << 8) );

    NRF_RADIO->CRCINIT = ((uint32_t)seed[0]) | ((uint32_t)seed[1])<<8 | ((uint32_t)seed[2])<<16;
    NRF_RADIO->TXPOWER = MAX_TX_POWER;
    NRF_RADIO->INTENSET = (RADIO_INTENSET_DISABLED_Enabled << RADIO_INTENSET_DISABLED_Pos);

    NVIC_ClearPendingIRQ(RADIO_IRQn);
    NVIC_EnableIRQ(RADIO_IRQn);
    NVIC_SetPriority(RADIO_IRQn, 0);
}

void ble_pdu_init(uint8_t* data)
{
    // Set PDU flags
    data[0] = 0x42;
    data[1] = 0;
    data[2] = 0;

    // Add device advertising addr
    if ( ( NRF_FICR->DEVICEADDR[0]           != 0xFFFFFFFF)
    ||   ((NRF_FICR->DEVICEADDR[1] & 0xFFFF) != 0xFFFF) )
    {
        data[BD_ADDR_OFFS    ] = (NRF_FICR->DEVICEADDR[0]      ) & 0xFF;
        data[BD_ADDR_OFFS + 1] = (NRF_FICR->DEVICEADDR[0] >>  8) & 0xFF;
        data[BD_ADDR_OFFS + 2] = (NRF_FICR->DEVICEADDR[0] >> 16) & 0xFF;
        data[BD_ADDR_OFFS + 3] = (NRF_FICR->DEVICEADDR[0] >> 24)       ;
        data[BD_ADDR_OFFS + 4] = (NRF_FICR->DEVICEADDR[1]      ) & 0xFF;
        data[BD_ADDR_OFFS + 5] = (NRF_FICR->DEVICEADDR[1] >>  8) & 0xFF;
    }
    else
    {
        static const uint8_t random_bd_addr[M_BD_ADDR_SIZE] = {0xE2, 0xA3, 0x01, 0xE7, 0x61, 0xF7};
        memcpy(&(data[BD_ADDR_OFFS]), &(random_bd_addr[0]), M_BD_ADDR_SIZE);
    }
    
    #ifdef LOG
    SEGGER_RTT_printf(0, "%u> BLE: Given advertise addr: %2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x\r\n", timer_get_seconds(), 
        data[BD_ADDR_OFFS], data[BD_ADDR_OFFS + 1],data[BD_ADDR_OFFS + 2], data[BD_ADDR_OFFS + 3], data[BD_ADDR_OFFS + 4], data[BD_ADDR_OFFS + 5]);
    #endif

    data[1] = (data[1] == 0) ? M_BD_ADDR_SIZE : data[1];
}
