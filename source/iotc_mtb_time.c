/*******************************************************************************
* Copyright 2020-2021, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/
//
// Copyright: Avnet 2021
// Modified by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//

#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "sntp.h"

#include "clock.h"
#include "cy_time.h"
#include "cyhal_rtc.h"
#include "iotconnect_common.h"

#ifndef IOTC_MTB_TIME_MAX_TRIES
#define IOTC_MTB_TIME_MAX_TRIES 10
#endif

static cyhal_rtc_t *cy_time = NULL;
static cyhal_rtc_t cy_timer_rtc;

static bool callback_received = false;

void iotc_set_system_time_us(u32_t sec, u32_t us) {
    cy_rslt_t result = CY_RSLT_SUCCESS;
    taskENTER_CRITICAL();
    if (cy_time == NULL) {
        result = cyhal_rtc_init(&cy_timer_rtc);
        CY_ASSERT(CY_RSLT_SUCCESS == result);
        cy_time = &cy_timer_rtc;
        cy_set_rtc_instance(cy_time); // becomes global clock
    }
    if (result == CY_RSLT_SUCCESS) {
        time_t secs_time_t = sec;
        result = cyhal_rtc_write(cy_time,  gmtime(&secs_time_t));
        CY_ASSERT(CY_RSLT_SUCCESS == result);
    }
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    callback_received = true;
    taskEXIT_CRITICAL();
}

time_t timenow = 0;
int iotc_mtb_time_obtain(const char *server) {
    u8_t reachable = 0;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, server);
    sntp_init();
    timenow = time(NULL);
    printf("Obtaining network time...");
    for (int i = 0; (reachable = sntp_getreachability(0)) == 0 && i < IOTC_MTB_TIME_MAX_TRIES; i++) {
        if (!reachable) {
            printf(".");
            vTaskDelay(pdMS_TO_TICKS(1000));
        } else {
            break;
        }
    }
    printf(".\n");
    if (!reachable) {
        printf("Unable to get time!\n");
        return -1;
    }
    if (!callback_received) {
        printf("No callback was received from SNTP module. Ensure that iotc_set_system_time_us is defined as SNTP_SET_SYSTEM_TIME_US callback!\n");
        return -1;
    }
    timenow = time(NULL);
    printf("Time received from NTP. Time now: %s!\n", iotcl_to_iso_timestamp(timenow));
    return 0;
}
