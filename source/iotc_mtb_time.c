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
#include "cyhal_rtc.h"

#include "iotcl_cfg.h"
#include "iotcl_util.h"

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
    char time_str_buffer[IOTCL_ISO_TIMESTAMP_STR_LEN + 1] = {0};
    iotcl_iso_timestamp_now(time_str_buffer, sizeof(time_str_buffer));
    printf("Time received from NTP. Time now: %s!\n", time_str_buffer);
    return 0;
}
