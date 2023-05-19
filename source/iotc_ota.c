/*-----------------------------------------------------------------
 Copyright: Avnet 2023
 Created by Shu Liu <shu.liu@avnet.com> on 05/15/23.
-----------------------------------------------------------------*/

/* Middleware libraries */
//#include "cy_retarget_io.h"
#include "cy_log.h"

/* OTA API */
#include "cy_ota_api.h"
#include "ota_serial_flash.h"

#include "iotc_ota.h"

#define HTTP_SERVER_PORT	443
/*******************************************************************************
Forward Declaration
********************************************************************************/
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data);

/*******************************************************************************
* Global Variables
********************************************************************************/
/* OTA context */
static cy_ota_context_ptr ota_context;

/* Network parameters for OTA */
static cy_ota_network_params_t ota_network_params =
{
    .http =
    {
        .server =
        {
            .host_name = NULL,
            .port = HTTP_SERVER_PORT
        },
        .file = NULL,

        .credentials =
        {
            .root_ca = DIGICERT_GLOBAL_ROOT_G2,
            .root_ca_size = sizeof(DIGICERT_GLOBAL_ROOT_G2),
        },
    },
    .use_get_job_flow = CY_OTA_DIRECT_FLOW,
    .initial_connection = CY_OTA_CONNECTION_HTTPS,
};

/* Parameters for OTA agent */
static cy_ota_agent_params_t ota_agent_params =
{
    .cb_func = ota_callback,
    .cb_arg = &ota_context,
    .reboot_upon_completion = 1,
    .validate_after_reboot = 1,
    .do_not_send_result = 1
};

bool start_ota(char* host, char* path)
{
   ota_network_params.http.file = path;
   ota_network_params.http.server.host_name = host;
   printf("\r\nHTTP FILE is %s\r\n", ota_network_params.http.file);

  if( cy_ota_agent_start(&ota_network_params, &ota_agent_params, &ota_context) != CY_RSLT_SUCCESS )
  {
    printf("\n Initializing and starting the OTA agent failed.\n");
    return false;
  }
  return true;
}

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
cy_ota_callback_results_t ota_callback(cy_ota_cb_struct_t *cb_data)
{
    cy_ota_callback_results_t   cb_result = CY_OTA_CB_RSLT_OTA_CONTINUE;
    const char                  *state_string;
    const char                  *error_string;

    if (cb_data == NULL)
    {
        return CY_OTA_CB_RSLT_OTA_STOP;
    }

    state_string  = cy_ota_get_state_string(cb_data->ota_agt_state);
    error_string  = cy_ota_get_error_string(cy_ota_get_last_error());

    switch (cb_data->reason)
    {

        case CY_OTA_LAST_REASON:
            break;

        case CY_OTA_REASON_SUCCESS:
            printf(">> APP CB OTA SUCCESS state:%d %s last_error:%s\n\n",
                    cb_data->ota_agt_state,
                    state_string, error_string);
            break;

        case CY_OTA_REASON_FAILURE:
            printf(">> APP CB OTA FAILURE state:%d %s last_error:%s\n\n",
                    cb_data->ota_agt_state, state_string, error_string);
            break;

        case CY_OTA_REASON_STATE_CHANGE:
            switch (cb_data->ota_agt_state)
            {
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
                    if ((cb_data->broker_server.host_name == NULL) ||
                        ( cb_data->broker_server.port == 0) ||
                        ( strlen(cb_data->file) == 0) )
                    {
                        printf("ERROR in callback data: HTTP: server: %p port: %d topic: '%p'\n",
                                cb_data->broker_server.host_name,
                                cb_data->broker_server.port,
                                cb_data->file);
                        cb_result = CY_OTA_CB_RSLT_OTA_STOP;
                    }
                    printf("HTTP: server:%s port: %d file: '%s'\n",
                            cb_data->broker_server.host_name,
                            cb_data->broker_server.port,
                            cb_data->file);

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
                    printf("APP CB OTA PARSE JOB: '%.*s' \n",
                    strlen(cb_data->json_doc),
                    cb_data->json_doc);
                    break;

                case CY_OTA_STATE_JOB_REDIRECT:
                    printf("APP CB OTA JOB REDIRECT\n");
                    break;

                case CY_OTA_STATE_DATA_CONNECT:
                    printf("APP CB OTA CONNECT FOR DATA using ");
                    printf("HTTP: %s:%d \n", cb_data->broker_server.host_name,
                    cb_data->broker_server.port);
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
                    printf("HTTP: Server:%s port: %d\n",
                            cb_data->broker_server.host_name,
                            cb_data->broker_server.port);
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
                    break;

                case CY_OTA_STATE_STORAGE_OPEN:
                    printf("APP CB OTA STORAGE OPEN\n");
                    break;

                case CY_OTA_STATE_STORAGE_WRITE:
                    printf("APP CB OTA STORAGE WRITE %ld%% (%ld of %ld)\n",
                            (unsigned long)cb_data->percentage,
                            (unsigned long)cb_data->bytes_written,
                            (unsigned long)cb_data->total_size);

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
            }   /* switch state */
            break;
    }
    return cb_result;
}


void ota_smif_init(void){
	if (ota_smif_initialize() != CY_RSLT_SUCCESS)
	{
		printf("ERROR returned from ota_smif_initialize()!!!!!\n");
	}
}

void ota_storage_validate(void){
	if(cy_ota_storage_validated() != CY_RSLT_SUCCESS){
		printf("\n Failed to validate the update.\n");
	}
}


