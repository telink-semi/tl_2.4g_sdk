/********************************************************************************************************
 * @file    app_common.h
 *
 * @brief   This is the header file for BLE SDK
 *
 * @author  BLE GROUP
 * @date    06,2022
 *
 * @par     Copyright (c) 2022, Telink Semiconductor (Shanghai) Co., Ltd. ("TELINK")
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
#ifndef VENDOR_COMMON_APP_COMMON_H_
#define VENDOR_COMMON_APP_COMMON_H_

#include "vendor/common/user_config.h"


/**
 * @brief Enables the common logging functionality for the application
 */
#ifndef     APP_LOG_COMMON_EN
#define     APP_LOG_COMMON_EN                            (1 & APP_LOG_EN)
#endif


/**
 * @brief       this function is used to set deepsleep retention SRAM size for all application project.
 *              select DEEPSLEEP_MODE_RET_SRAM_** according to retention data size used.
 * @param[in]   none
 * @return      none
 */
void blc_app_setDeepsleepRetentionSramSize(void);


/**
 * @brief      This function is check If IRK(local IRK or peer IRK) valid
 * @param[in]  pIrk - pointer to IRK(local IRK or peer IRK)
 * @return     1: IRK valid
 *             0: IRK invalid
 */
int blc_app_isIrkValid(u8* pIrk);


#endif /* VENDOR_COMMON_APP_COMMON_H_ */
