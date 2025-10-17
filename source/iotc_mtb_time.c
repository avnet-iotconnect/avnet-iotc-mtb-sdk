/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include <time.h>

#include "FreeRTOS.h"
#include "task.h"

#include "sntp.h"

#include "clock.h"
#include "cy_time.h"

#include "iotcl_cfg.h"
#include "iotcl_util.h"

#ifndef IOTC_MTB_TIME_MAX_TRIES
#define IOTC_MTB_TIME_MAX_TRIES 15
#endif

#if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
static mtb_hal_rtc_t* mtb_time_rtc_ptr;
#else
static cyhal_rtc_t cy_time_rtc_inst;
#endif

static bool callback_received = false;

void iotc_set_system_time_us(u32_t sec, u32_t us) {
    cy_rslt_t result;
    time_t secs_time_t = sec;

    taskENTER_CRITICAL();
    /* HAL API version 2 and lower support dynamically allocating and initializing
    * an RTC instance if one is not already set. HAL API version 3 requires that
    * the RTC instance be allocated in the configurator and configured prior to this
    * function being called */
    #if defined(MTB_HAL_API_VERSION) && ((MTB_HAL_API_VERSION) >= 3)
    mtb_time_rtc_ptr = mtb_clib_support_get_rtc();
    CY_ASSERT(NULL != mtb_time_rtc_ptr);
    result = mtb_hal_rtc_write(mtb_time_rtc_ptr, gmtime(&secs_time_t));
    #else /* Older HAL versions define CYHAL_API_VERSION */
    result = cyhal_rtc_init(&cy_time_rtc_inst);
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    result = cyhal_rtc_write(&cy_time_rtc_inst, gmtime(&secs_time_t));
    #endif
    CY_ASSERT(CY_RSLT_SUCCESS == result);
    callback_received = true;
    taskEXIT_CRITICAL();
}


int iotc_mtb_time_obtain(const char *server) {
    u8_t reachable = 0;
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, server);
    sntp_init();
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
    printf("Time received from NTP. Time now: %ld\n", (long int)time(NULL));
    return 0;
}
