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

#include "FreeRTOS.h"
#include "task.h"
#include <cy_http_client_api.h>

#include "iotconnect_discovery.h"
#include "iotc_http_client.h"
#include "iotconnect_certs.h"

#ifndef IOTC_HTTP_SEND_RECV_TIMEOUT_MS
#define IOTC_HTTP_SEND_RECV_TIMEOUT_MS    ( 10000 )
#endif

#ifndef IOTC_HTTP_BUFFER_SIZE
#define IOTC_HTTP_BUFFER_SIZE    ( 3000 )
#endif

#ifndef IOTC_HTTP_CONNECT_MAX_RETRIES
#define IOTC_HTTP_CONNECT_MAX_RETRIES    ( 5 )
#endif

static uint8_t http_client_buffer[IOTC_HTTP_BUFFER_SIZE];

unsigned int iotconnect_https_request(IotConnectHttpResponse *response, const char *host, const char *path,
        const char *send_str) {
    cy_rslt_t res = 0;
    cy_awsport_ssl_credentials_t credentials;
    cy_awsport_server_info_t server_info;
    cy_http_client_t handle;
    cy_http_client_request_header_t request;
    cy_http_client_header_t header[2];
    cy_http_client_response_t client_resp;

    (void) memset(&credentials, 0, sizeof(credentials));
    (void) memset(&server_info, 0, sizeof(server_info));
    server_info.host_name = host;
    server_info.port = 443;

    credentials.root_ca = CERT_GODADDY_ROOT_CA;
    credentials.root_ca_size = strlen(CERT_GODADDY_ROOT_CA) + 1; // needs to include the null
    credentials.root_ca_verify_mode = CY_AWS_ROOTCA_VERIFY_REQUIRED;
    credentials.sni_host_name = host;
    credentials.sni_host_name_size = strlen(host) + 1; // needs to include the null

    response->data = NULL;

    res = cy_http_client_init();
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to init the http client. Error=0x%08lx\n", res);
        return res;
    }

    res = cy_http_client_create(&credentials, &server_info, NULL, NULL, &handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to create the http client. Error=0x%08lx.\n", res);
        goto cleanup_deinit;
    }

    int i = IOTC_HTTP_CONNECT_MAX_RETRIES;
    do {
        res = cy_http_client_connect(handle, IOTC_HTTP_SEND_RECV_TIMEOUT_MS, IOTC_HTTP_SEND_RECV_TIMEOUT_MS);
        i--;
        if (res != CY_RSLT_SUCCESS) {
            printf("Failed to connect to http server. Error=0x%08lx. ", res);
            if (i <= 0) {
                printf("Giving up! Max retry count %d reached", IOTC_HTTP_CONNECT_MAX_RETRIES);
                goto cleanup_delete;
            } else {
                printf("Retrying...\n");
                vTaskDelay(pdMS_TO_TICKS(2000));;
            }
        }
    } while (res != CY_RSLT_SUCCESS);

    request.buffer = http_client_buffer;
    request.buffer_len = IOTC_HTTP_BUFFER_SIZE;
    request.headers_len = 0;
    request.method = (send_str ? CY_HTTP_CLIENT_METHOD_POST : CY_HTTP_CLIENT_METHOD_GET);
    request.range_end = -1;
    request.range_start = 0;
    request.resource_path = path;

    uint32_t num_headers = 0;
    header[num_headers].field = "Connection";
    header[num_headers].field_len = strlen("Connection");
    header[num_headers].value = "close";
    header[num_headers].value_len = strlen("close");
    num_headers++;
    header[num_headers].field = "Content-Type";
    header[num_headers].field_len = strlen("Content-Type");
    header[num_headers].value = "application/json";
    header[num_headers].value_len = strlen("application/json");
    num_headers++;

    /* Generate the standard header and user-defined header, and update in the request structure. */
    res = cy_http_client_write_header(handle, &request, &header[0], num_headers);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed write HTTP headers. Error=0x%08lx\n", res);
        goto cleanup_disconnect;
    }
    /* Send the HTTP request and body to the server and receive the response from it. */
    res = cy_http_client_send(handle, &request, (uint8_t*) send_str, (send_str ? strlen(send_str) : 0), &client_resp);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed send the HTTP request. Error=0x%08lx\n", res);
        goto cleanup_disconnect;
    }

    response->data = malloc(client_resp.body_len + 1);
    if (!response->data) {
        printf("Failed to malloc response data\n");
        goto cleanup_disconnect;
    }
    memcpy(response->data, client_resp.body, client_resp.body_len);
    response->data[client_resp.body_len] = 0; // terminate the string

    cleanup_disconnect: res = cy_http_client_disconnect(handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to disconnect the HTTP client\n");
    }

    cleanup_delete: res = cy_http_client_delete(handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to delete the HTTP client\n");
    }

    cleanup_deinit: res = cy_http_client_deinit();
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to deinit the HTTP client\n");
    }
    return (unsigned int) res;
}

void iotconnect_free_https_response(IotConnectHttpResponse *response) {
    if (response->data) {
        free(response->data);
        response->data = NULL;
    }
}

