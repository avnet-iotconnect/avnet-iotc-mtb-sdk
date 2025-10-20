/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_MTB_TIME_H
#define IOTC_MTB_TIME_H

// For u32_t
#include "lwip/arch.h"

#ifdef __cplusplus
extern "C" {
#endif

// callback for sntp.c
void iotc_set_system_time_us(u32_t sec, u32_t us);

// invoke to obtain time via SNTP, once the network is up
int iotc_mtb_time_obtain(const char *server);

#ifdef __cplusplus
}
#endif

#endif
