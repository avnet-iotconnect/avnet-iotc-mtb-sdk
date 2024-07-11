/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Shu Liu <shu.liu@avnet.com>, Nik Markovic<nikola.markovic@avnet.com> et al.
 */


// Surround the whole OTA support around IOTC_OTA_SUPPORT
// To avoid potential issues with older BSPs if OTA is not supported
#ifdef IOTC_OTA_SUPPORT

#include "cyabs_rtos.h"
#include "cy_log.h"
#include "cy_ota_api.h"
#include "cy_ota_storage_api.h"

#include "iotcl_certs.h"
#include "iotcl.h"
#include "iotcl_util.h"

#include "iotc_ota.h"

/* Application ID */
#define APP_ID                              (0)
#define HTTP_SERVER_PORT	443



// Captured task handle that will will resume in case of synchronous OTA
static TaskHandle_t app_task_handle;


// Captured last error received at the time of OTA session completion
static cy_rslt_t last_session_result = CY_RSLT_OTA_ERROR_GENERAL; // assume a failure unless we get CY_OTA_STATE_OTA_COMPLETE

static cy_ota_context_ptr ota_context;

static cy_ota_storage_interface_t ota_interfaces = {
   .ota_file_open            = cy_ota_storage_open,
   .ota_file_read            = cy_ota_storage_read,
   .ota_file_write           = cy_ota_storage_write,
   .ota_file_close           = cy_ota_storage_close,
   .ota_file_verify          = cy_ota_storage_verify,
   .ota_file_validate        = cy_ota_storage_image_validate,
   .ota_file_get_app_info    = cy_ota_storage_get_app_info
};

static cy_ota_agent_params_t ota_agent_params = {
    // .cb_func = usr_ota_cb ? usr_ota_cb : iotc_ota_callback,
	.cb_func = NULL,
    .cb_arg = &ota_context,
    .reboot_upon_completion = 1, /* Reboot after completing OTA with success. */
    .validate_after_reboot = 1,
    .do_not_send_result = 1
};

static cy_ota_network_params_t ota_network_params = {
    .http = {
        .server = {
            .host_name = NULL,
            .port = HTTP_SERVER_PORT
        },
        .file = NULL,
        .credentials = {
            .root_ca = NULL,
            .root_ca_size = 0,
        },
    },
    .use_get_job_flow = CY_OTA_DIRECT_FLOW,
    .initial_connection = CY_OTA_CONNECTION_HTTPS
};

/*******************************************************************************
 * Function Name: ota_callback()
 *******************************************************************************
 * Summary:
 *  Prints the status of the OTA agent on every event. This callback is optional,
 *  but be aware that the OTA middleware will not print the status of OTA agent
 *  on its own.
 *
 * Return:
 *  CY_OTA_CB_RSLT_OTA_CONTINUE - OTA Agent to continue with function.
 *  CY_OTA_CB_RSLT_OTA_STOP     - OTA Agent to End current update session.
 *  CY_OTA_CB_RSLT_APP_SUCCESS  - Application completed task, success.
 *  CY_OTA_CB_RSLT_APP_FAILED   - Application completed task, failure.
 *
 *******************************************************************************/
static cy_ota_callback_results_t iotc_ota_callback(cy_ota_cb_struct_t *cb_data) {
	cy_ota_callback_results_t cb_result = CY_OTA_CB_RSLT_OTA_CONTINUE;
	const char *state_string;
	const char *error_string;

	if (cb_data == NULL) {
		return CY_OTA_CB_RSLT_OTA_STOP;
	}

	state_string = cy_ota_get_state_string(cb_data->ota_agt_state);
	error_string = cy_ota_get_error_string(cy_ota_get_last_error());

	switch (cb_data->reason) {

	case CY_OTA_LAST_REASON:
		break;

	case CY_OTA_REASON_SUCCESS:
		printf(">> APP CB OTA SUCCESS state:%d %s last_error:%s\n\n", cb_data->ota_agt_state, state_string,
				error_string);
		break;

	case CY_OTA_REASON_FAILURE:
		printf(">> APP CB OTA FAILURE state:%d %s last_error:%s\n\n", cb_data->ota_agt_state, state_string,
				error_string);
		break;

	case CY_OTA_REASON_STATE_CHANGE:
		switch (cb_data->ota_agt_state) {
		case CY_OTA_STATE_NOT_INITIALIZED:
		case CY_OTA_STATE_EXITING:
		case CY_OTA_STATE_INITIALIZING:
		case CY_OTA_STATE_AGENT_STARTED:
		case CY_OTA_STATE_AGENT_WAITING:
			break;

		case CY_OTA_STATE_START_UPDATE:
			printf("APP CB OTA STATE CHANGE CY_OTA_STATE_START_UPDATE\n");
			break;

		case CY_OTA_STATE_JOB_CONNECT:
			printf("APP CB OTA CONNECT FOR JOB using ");
			/* NOTE:
			 *  HTTP - json_doc holds the HTTP "GET" request
			 */
			if ((cb_data->broker_server.host_name == NULL) || (cb_data->broker_server.port == 0)
					|| (strlen(cb_data->file) == 0)) {
				printf("ERROR in callback data: HTTP: server: %p port: %d topic: '%p'\n",
						cb_data->broker_server.host_name, cb_data->broker_server.port, cb_data->file);
				cb_result = CY_OTA_CB_RSLT_OTA_STOP;
			}
			printf("HTTP: server:%s port: %d file: '%s'\n", cb_data->broker_server.host_name,
					cb_data->broker_server.port, cb_data->file);

			break;

		case CY_OTA_STATE_JOB_DOWNLOAD:
			printf("APP CB OTA JOB DOWNLOAD using ");
			/* NOTE:
			 *  HTTP - json_doc holds the HTTP "GET" request
			 */
			printf("HTTP: '%s'\n", cb_data->file);
			break;

		case CY_OTA_STATE_JOB_DISCONNECT:
			printf("APP CB OTA JOB DISCONNECT\n");
			break;

		case CY_OTA_STATE_JOB_PARSE:
			printf("APP CB OTA PARSE JOB: '%.*s' \n", strlen(cb_data->json_doc), cb_data->json_doc);
			break;

		case CY_OTA_STATE_JOB_REDIRECT:
			printf("APP CB OTA JOB REDIRECT\n");
			break;

		case CY_OTA_STATE_DATA_CONNECT:
			printf("APP CB OTA CONNECT FOR DATA using ");
			printf("HTTP: %s:%d \n", cb_data->broker_server.host_name, cb_data->broker_server.port);
			break;

		case CY_OTA_STATE_DATA_DOWNLOAD:
			printf("APP CB OTA DATA DOWNLOAD using ");
			/* NOTE:
			 *  HTTP - json_doc holds the HTTP "GET" request
			 */
			printf("HTTP: '%.*s' ", strlen(cb_data->json_doc), cb_data->json_doc);
			printf("File: '%s'\n\n", cb_data->file);
			break;

		case CY_OTA_STATE_DATA_DISCONNECT:
			printf("APP CB OTA DATA DISCONNECT\n");
			break;

		case CY_OTA_STATE_RESULT_CONNECT:
			printf("APP CB OTA SEND RESULT CONNECT using ");
			/* NOTE:
			 *  HTTP - json_doc holds the HTTP "GET" request
			 */
			printf("HTTP: Server:%s port: %d\n", cb_data->broker_server.host_name, cb_data->broker_server.port);
			break;

		case CY_OTA_STATE_RESULT_SEND:
			printf("APP CB OTA SENDING RESULT using ");
			/* NOTE:
			 *  HTTP - json_doc holds the HTTP "PUT"
			 */
			printf("HTTP: '%s' \n", cb_data->json_doc);
			break;

		case CY_OTA_STATE_RESULT_RESPONSE:
			printf("APP CB OTA Got Result response\n");
			break;

		case CY_OTA_STATE_RESULT_DISCONNECT:
			printf("APP CB OTA Result Disconnect\n");
			break;

		case CY_OTA_STATE_OTA_COMPLETE:
			printf("APP CB OTA Session Complete\n");
			last_session_result = cy_ota_get_last_error();
			xTaskNotifyGive(app_task_handle);
			cb_result = CY_OTA_CB_RSLT_OTA_STOP;
			break;

		case CY_OTA_STATE_STORAGE_OPEN:
			printf("APP CB OTA STORAGE OPEN\n");
			break;

		case CY_OTA_STATE_STORAGE_WRITE:
			printf("APP CB OTA STORAGE WRITE %ld%% (%ld of %ld)\n", (unsigned long) cb_data->percentage,
					(unsigned long) cb_data->bytes_written, (unsigned long) cb_data->total_size);

			/* Move cursor to previous line */
			printf("\x1b[1F");
			break;

		case CY_OTA_STATE_STORAGE_CLOSE:
			printf("APP CB OTA STORAGE CLOSE\n");
			break;

		case CY_OTA_STATE_VERIFY:
			printf("APP CB OTA VERIFY\n");
			break;

		case CY_OTA_STATE_RESULT_REDIRECT:
			printf("APP CB OTA RESULT REDIRECT\n");
			break;

		case CY_OTA_NUM_STATES:
			break;
		} /* switch state */
		break;
	}
	return cb_result;
}

cy_rslt_t iotc_ota_init(void) {
	cy_rslt_t result = cy_ota_storage_init();
	if (result != CY_RSLT_SUCCESS) {
		printf("ERROR returned from ota_smif_initialize()!\n");
	}
	return result;
}

cy_rslt_t iotc_ota_storage_validated(void) {
	cy_rslt_t result = cy_ota_storage_image_validate(APP_ID);
	if (result != CY_RSLT_SUCCESS) {
		printf("Failed to flag firmware as valid.\n");
	}
	return result;
}

cy_rslt_t iotc_ota_start(IotConnectConnectionType connection_type, const char *host, const char *path, cy_ota_callback_t usr_ota_cb) {
	if (path == NULL || host == NULL) {
		return -1;
	}

	switch(connection_type) {
		case IOTC_CT_AWS:
			ota_network_params.http.credentials.root_ca = IOTCL_AMAZON_ROOT_CA1;
			ota_network_params.http.credentials.root_ca_size = sizeof(IOTCL_AMAZON_ROOT_CA1);
			break;
		case IOTC_CT_AZURE:
			ota_network_params.http.credentials.root_ca = IOTCL_CERT_DIGICERT_GLOBAL_ROOT_G2;
			ota_network_params.http.credentials.root_ca_size = sizeof(IOTCL_CERT_DIGICERT_GLOBAL_ROOT_G2);
			break;
		default:
			printf("Error: OTA Connection Type invalid!\n");
			return -2;

	}

	last_session_result = CY_RSLT_OTA_ERROR_GENERAL; // assume a failure unless we get CY_OTA_STATE_OTA_COMPLETE
	iotc_ota_cleanup();

	ota_agent_params.cb_func = usr_ota_cb ? usr_ota_cb : iotc_ota_callback;

	ota_network_params.http.file = iotcl_strdup(path);
	ota_network_params.http.server.host_name = iotcl_strdup(host);

	cy_rslt_t result = cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_interfaces, &ota_context);
	if (CY_RSLT_SUCCESS != result) {
		printf("Initializing and starting the OTA agent failed. Error is %lu\n", result);
		iotc_ota_cleanup();
	}
	return result;
}

cy_rslt_t iotc_ota_run(IotConnectConnectionType connection_type, const char *host, const char *path, cy_ota_callback_t usr_ota_cb) {
	cy_rslt_t result = iotc_ota_start(connection_type, host, path, usr_ota_cb);
	if (CY_RSLT_SUCCESS != result) {
		// OTA did not start. The called function will print the error.
		return result;
	}
	app_task_handle = xTaskGetCurrentTaskHandle();
	// wait for OTA completion
	ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(60 * 60 * 1000)); // one hour

	iotc_ota_cleanup();

	// the user should call
	return result;
}

cy_rslt_t iotc_ota_get_download_status(void) {
	return last_session_result;
}

const char* iotc_ota_get_download_error_string(void) {
	if (CY_RSLT_SUCCESS != last_session_result) {
		return cy_ota_get_error_string(last_session_result);
	} else {
		return NULL;
	}
}

void iotc_ota_system_reset(void) {
	// we don't have support for ThreadX, but there just in case we add it
#ifdef COMPONENT_THREADX
	cyhal_system_reset_device();
#else
	NVIC_SystemReset();
#endif
}

void iotc_ota_cleanup() {
	ota_agent_params.cb_func = NULL;
	if (ota_network_params.http.file) {
		iotcl_free((char *) ota_network_params.http.file);
		ota_network_params.http.file = NULL;
	}
	if (ota_network_params.http.server.host_name) {
		iotcl_free((char *) ota_network_params.http.server.host_name);
		ota_network_params.http.server.host_name = NULL;
	}
}

#endif // IOTC_OTA_SUPPORT
