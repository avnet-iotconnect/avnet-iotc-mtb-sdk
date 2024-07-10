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

// Call this only once from the application.
cy_rslt_t iotc_ota_init(void);

// Validate the update so we do not revert.
// After OTA update is successful, the board reboots and the new firmware that comes up needs
// to make this call after performing self-test to indicate to the bootloader that the image is healthy.
// Otherwise, the bootloader will swap back to the original firmware.
cy_rslt_t iotc_ota_storage_validated(void);

// Synchronous call to start the OTA agent, that will block until the OTA agent completes.
// The call will return the status or the initial agent start call.
// The user must get the download status/error to determine whether the download ultimately succeeded.
// Unlike the iotc_ota_start case, the user does not need to call iotc_ota_cleanup(). It will be handled internally.
// The callback is optional. If not provided, this module will handle OTA for the user by printing status messages.
cy_rslt_t iotc_ota_run(IotConnectConnectionType connection_type, const char* host, const char* path, cy_ota_callback_t usr_ota_cb);

// While the synchronous implementation is recommended, the asynchronous OTA download function is provided as well.
// The callback is optional. If not provided, this module will handle OTA for the user by printing status messages
// See the callback implementation in iotc_ota.c on how to get OTA result and stop it from infinitely trying.
// Ensure to call iotc_ota_cleanup() to clean up allocated URL strings once the OTA agent task completes.
cy_rslt_t iotc_ota_start(IotConnectConnectionType connection_type, const char* host, const char* path, cy_ota_callback_t usr_ota_cb);

// Call this if you ran the asynchronous iotc_ota_start when OTA agent task completes.
void iotc_ota_cleanup();

// Returns last error and status of the OTA download
cy_rslt_t iotc_ota_get_dowload_status(void);
const char* iotc_ota_get_download_error_string(void);

// Once OTA has been successful, issue a system reset
void iotc_ota_system_reset(void);

#endif // IOTC_OTA_SUPPORT

#endif // IOTC_OTA_H
