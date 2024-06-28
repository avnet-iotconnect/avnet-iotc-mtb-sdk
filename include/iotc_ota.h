/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Shu Liu <shu.liu@avnet.com> et al.
 */

#ifndef IOTC_OTA_H
#define IOTC_OTA_H


// Surround the whole OTA support around IOTC_OTA_SUPPORT
// To avoid potential issues with older BSPs if OTA is not supported
#ifdef IOTC_OTA_SUPPORT

#include "iotconnect.h"
#include "cy_ota_api.h"

// Call this once in the application
cy_rslt_t iotc_ota_init(void);

// Validate the update so we do not revert.
// After ota update is successful, the board reboots and the new firmware that comes up needs
// to make this call after performing self-test to indicate to the bootloader that the image is healthy.
// Otherwise, the bootloader will swap back to the original firmware.
cy_rslt_t iotc_ota_storage_validated(void);

// The callback is optional. If not provided, this module will handle OTA for the user by printing status messages
cy_rslt_t iotc_ota_start(IotConnectConnectionType connection_type, const char* host, const char* path, cy_ota_callback_t usr_ota_cb);

#endif // IOTC_OTA_SUPPORT

#endif // IOTC_OTA_H
