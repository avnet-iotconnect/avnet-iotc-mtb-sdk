//
// Copyright: Avnet 2021
// Created by Nik Markovic <nikola.markovic@avnet.com> on 11/11/21.
//

#ifndef IOTC_MTB_TIME_H
#define IOTC_MTB_TIME_H

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
