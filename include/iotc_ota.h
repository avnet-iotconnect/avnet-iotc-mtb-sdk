/*-----------------------------------------------------------------
 Copyright: Avnet 2023
 Created by Shu Liu <shu.liu@avnet.com> on 05/15/23.
-----------------------------------------------------------------*/
#include "cy_ota_api.h"

// Call this once in the application
cy_rslt_t iotc_ota_init(void);

// Validate the update so we do not revert
cy_rslt_t iotc_ota_storage_validated(void);

cy_rslt_t iotc_ota_start(char* host, char* path, cy_ota_callback_t usr_ota_cb);


