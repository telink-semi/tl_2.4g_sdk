/********************************************************************************************************
 * @file    tlkapp_srx.c
 *
 * @brief   This is the source file for 2.4G SDK
 *
 * @author  2.4G Group
 * @date    2024
 *
 * @par     Copyright (c) 2024, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
 *
 *          Licensed under the Apache License, Version 2.0 (the "License");
 *          you may not use this file except in compliance with the License.
 *          You may obtain a copy of the License at
 *
 *              http://www.apache.org/licenses/LICENSE-2.0
 *
 *          Unless required by applicable law or agreed to in writing, software
 *          distributed under the License is distributed on an "AS IS" BASIS,
 *          WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *          See the License for the specific language governing permissions and
 *          limitations under the License.
 *
 *******************************************************************************************************/
#include "vendor/2p4g_gen_fsk/tlkapp_config.h"
#include "tl_common.h"
#include "drivers.h"
#include "../stack/2p4g/genfsk_ll/genfsk_ll.h"
#if (GEN_FSK_MODE == GEN_FSK_SRX)

#define    PRI_FLT_MODE_EN                0

#define    RX_BUF_LEN                     128
#define    RX_BUF_NUM                     4
static unsigned char rx_buf[RX_BUF_LEN*RX_BUF_NUM] __attribute__ ((aligned (4))) = {};
static volatile unsigned char rx_flag = 0;
static volatile unsigned char rx_first_timeout_flag = 0;
static volatile unsigned char irq_cnt_rx_first_timeout = 0;
static unsigned char *rx_packet = 0;
static unsigned char rx_payload_len = 0;
static volatile unsigned char *rx_payload = 0;
static volatile unsigned char rssi = 0;
static volatile unsigned int rx_timestamp = 0;
static volatile unsigned int irq_cnt_rx_crc_ok = 0;
static volatile unsigned int irq_cnt_rx = 0;
static volatile unsigned int irq_cnt_unmatch=0;
static volatile unsigned int irq_cnt_match=0;

//crc_config
rf_crc_config_t crc_config = {
    .init_value = 0xffffffff,
    .poly = 0x00001021,
    .xor_out = 0,
    .byte_order = 1,
    .start_cal_pos = 0,
    .len = 2,
};

/**
 *  @note  The TPLL_GenericHeader_t function to set generic packet header,
 *         You need to configure the size in bit and the value of h0,h1 and payload len.
 *         You also can configure the start location of pid and no ack bit if you need.
 *         only generic mode need to configure this!!!
*/
gen_fsk_generic_header_t GEN_FSK_GenericHeader = {
        .h0_size = 0,
        .length_size = 8,
        .h1_size = 0,
        .h0_val = 0,
        .length_val = 8,
        .h1_val = 0,
};

/**
 *  @note  The rf_pkt_flt function matches from the high bit,if filter filed have the bytes you want than will be matched.
 *         You need to configure rf_pkt_mask and rf_pkt_match from the high bit when configuring it,
 *         the below takes generic packet for example, have the fix packet could not use this function!
*/
rf_pkt_flt_t GEN_FSK_PktFlt = {
        .rf_pkt_flt_start = 0,//start at h0
        .rf_pkt_flt_end = 9,
        .rf_pkt_match_threshold = 24,//max 64 bit
        .rf_pkt_match_low=0x00000000,
        .rf_pkt_match_high=0x00030201,
        .rf_pkt_mask_low=0x00000000,
        .rf_pkt_mask_high=0x00ffffff,//if bit is 1 means this bit should matched
};

_attribute_ram_code_sec_ void rf_irq_handler(void)
{
    if (rf_get_irq_status(FLD_RF_IRQ_RX)){
        rx_packet = rf_get_rx_packet_addr(RX_BUF_NUM,RX_BUF_LEN,rx_buf);
        irq_cnt_rx++;
        if(!rf_get_crc_err()){
            irq_cnt_rx_crc_ok++;
#if UI_LED_ENABLE
            gpio_toggle(GPIO_LED_BLUE);
#endif
        }
        rx_flag = 1;
        rf_clr_irq_status(FLD_RF_IRQ_RX );
    }
    else if (rf_get_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT)){
        rx_first_timeout_flag = 1;
        irq_cnt_rx_first_timeout++;
        rf_clr_irq_status(FLD_RF_IRQ_FIRST_TIMEOUT );
    }else if(rf_get_irq_status(FLD_RF_IRQ_PKT_UNMATCH )){
        irq_cnt_unmatch++;
        rf_clr_irq_status(FLD_RF_IRQ_PKT_UNMATCH);
    }
    else if(rf_get_irq_status(FLD_RF_IRQ_PKT_MATCH )){
        irq_cnt_match++;
        rf_clr_irq_status(FLD_RF_IRQ_PKT_MATCH);
    }
    else{
        rf_clr_irq_status(FLD_RF_IRQ_ALL);
    }
}
PLIC_ISR_REGISTER(rf_irq_handler, IRQ_ZB_RT)

void user_gpio_init(void)
{

}

/**
 * @brief       user initialization when MCU wake_up from deepSleep_retention mode
 * @param[in]   none
 * @return      none
 */
void user_init_deepRetn(void)
{

}

void user_init_normal(void)
{
    user_gpio_init();

#if (TLKAPI_DEBUG_ENABLE)
    tlkapi_debug_init();
#endif
    unsigned char sync_word[7] = {0x53, 0x78, 0x56, 0x52};
    //generic FSK Link Layer configuratioin
    gen_fsk_packet_format_set(GEN_FSK_MODE_FIXED_FORMAT,8);
    gen_fsk_datarate_set(GEN_FSK_DATARATE_1MBPS); //Note that this API must be invoked first before all other APIs
    gen_fsk_preamble_len_set(4);
    gen_fsk_sync_word_len_set(SYNC_WORD_LEN_4BYTE);
    gen_fsk_sync_word_set(GEN_FSK_PIPE0, sync_word); //set pipe0's sync word
    gen_fsk_pipe_open(GEN_FSK_PIPE0); //enable pipe0's reception
    gen_fsk_tx_pipe_set(GEN_FSK_PIPE0); //set pipe0 as the TX pipe
    gen_fsk_set_crc_config(&crc_config);
    gen_fsk_radio_power_set(RF_POWER_INDEX_N0p07dBm);
    gen_fsk_rx_buffer_set(rx_buf, RX_BUF_NUM, RX_BUF_LEN);
    gen_fsk_channel_set(17); //set rf freq as 2408MHz,1M step
    gen_fsk_radio_state_set(GEN_FSK_STATE_AUTO); //set transceiver to basic RX state
#if(PRI_FLT_MODE_EN)
    gen_fsk_set_pkt_filter(GEN_FSK_PktFlt);
#endif
    gen_fsk_rx_settle_set(89);
    delay_us(90); //wait for rx settle

    //irq configuration
    plic_set_priority(IRQ_ZB_RT, 3);
    plic_interrupt_enable(IRQ_ZB_RT);
    rf_set_irq_mask(FLD_RF_IRQ_RX | FLD_RF_IRQ_FIRST_TIMEOUT | FLD_RF_IRQ_PKT_MATCH | FLD_RF_IRQ_PKT_UNMATCH);
    rf_clr_irq_status(FLD_RF_IRQ_ALL);
    core_interrupt_enable();

    //start the SRX
    gen_fsk_srx_start(rf_stimer_get_tick()+50*RF_SYSTEM_TIMER_TICK_1US, 0); //RX first timeout is disabled and the transceiver won't exit the RX state until a packet arrives
}
_attribute_no_inline_ void main_loop (void)
{
    ////////////////////////////////////// Debug entry /////////////////////////////////
    #if (TLKAPI_DEBUG_ENABLE)
        tlkapi_debug_handler();
    #endif

    if (rx_flag) {
        rx_flag = 0;
        rx_payload = gen_fsk_rx_payload_get(rx_packet, &rx_payload_len);
        rssi = (gen_fsk_rx_packet_rssi_get(rx_packet) + 110);
        rx_timestamp = gen_fsk_rx_timestamp_get(rx_packet);
#if UI_LED_ENABLE
        gpio_toggle(GPIO_LED_GREEN);
#endif
        //start the SRX
        gen_fsk_srx_start(rf_stimer_get_tick()+50*RF_SYSTEM_TIMER_TICK_1US, 0);
    }
    if (rx_first_timeout_flag) {
        rx_first_timeout_flag = 0;
        //start the SRX
        gen_fsk_srx_start(rf_stimer_get_tick()+50*RF_SYSTEM_TIMER_TICK_1US, 0);
    }
}
#endif
