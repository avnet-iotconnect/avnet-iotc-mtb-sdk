/* SPDX-License-Identifier: MIT
 * Copyright (C) 2025 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#include "FreeRTOS.h"
#include "task.h"

#include <cy_http_client_api.h>

#include "iotcl_certs.h"
#include "iotc_http_client.h"

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

unsigned int iotconnect_https_request_with_opts(IotConnectHttpResponse *response, const char *host, const char *path,
        const char *send_str, IotConnectHttpOpts *opts) {
    cy_rslt_t res = 0;
    cy_awsport_ssl_credentials_t credentials;
    cy_awsport_server_info_t server_info;
    cy_http_client_t handle;
    cy_http_client_request_header_t request = {0};
    cy_http_client_response_t client_resp;
    cy_http_client_header_t header[IOTC_HTTP_HEADERS_MAX + 2];

    (void) memset(&credentials, 0, sizeof(credentials));
    (void) memset(&server_info, 0, sizeof(server_info));
    server_info.host_name = host;
    server_info.port = 443;

    // Use caller-supplied CA cert or fall back to GoDaddy G2
    const char *ca_cert = (opts && opts->ca_cert) ? opts->ca_cert : IOTCL_CERT_GODADDY_SECURE_SERVER_CERTIFICATE_G2;
    credentials.root_ca = ca_cert;
    credentials.root_ca_size = strlen(ca_cert) + 1; // needs to include the null
    credentials.root_ca_verify_mode = CY_AWS_ROOTCA_VERIFY_REQUIRED;
    credentials.sni_host_name = host;
    credentials.sni_host_name_size = strlen(host) + 1; // needs to include the null

    // Populate mTLS credentials if provided
    if (opts && opts->cert && opts->key) {
        credentials.client_cert = opts->cert;
        credentials.client_cert_size = strlen(opts->cert) + 1; // needs to include the null
        credentials.private_key = opts->key;
        credentials.private_key_size = strlen(opts->key) + 1; // needs to include the null
    }

    response->data = NULL;

    res = cy_http_client_init();
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to init the http client. Error=0x%08x\n", (unsigned int) res);
        return res;
    }

    res = cy_http_client_create(&credentials, &server_info, NULL, NULL, &handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to create the http client. Error=0x%08x.\n", (unsigned int) res);
        goto cleanup_deinit;
    }

    int i = IOTC_HTTP_CONNECT_MAX_RETRIES;
    do {
        res = cy_http_client_connect(handle, IOTC_HTTP_SEND_RECV_TIMEOUT_MS, IOTC_HTTP_SEND_RECV_TIMEOUT_MS);
        i--;
        if (res != CY_RSLT_SUCCESS) {
            printf("Failed to connect to http server. Error=0x%08x. ", (unsigned int) res);
            if (i <= 0) {
                printf("Giving up! Max retry count %d reached\n", IOTC_HTTP_CONNECT_MAX_RETRIES);
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
    request.range_start = -1;
    request.resource_path = path;

    // 2 fixed headers + optional extra headers from opts (capped at IOTC_HTTP_HEADERS_MAX)
    size_t extra_headers = (opts && opts->headers && opts->headers_len > 0) ? opts->headers_len : 0;
    if (extra_headers > IOTC_HTTP_HEADERS_MAX) {
        printf("Too many extra HTTP headers (%zu), capping at %d\n", extra_headers, IOTC_HTTP_HEADERS_MAX);
        extra_headers = IOTC_HTTP_HEADERS_MAX;
    }

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

    // Append caller-supplied headers
    for (size_t j = 0; j < extra_headers; j++) {
        header[num_headers].field = opts->headers[j].name;
        header[num_headers].field_len = strlen(opts->headers[j].name);
        header[num_headers].value = opts->headers[j].value;
        header[num_headers].value_len = strlen(opts->headers[j].value);
        num_headers++;
    }

    /* Generate the standard header and user-defined header, and update in the request structure. */
    res = cy_http_client_write_header(handle, &request, &header[0], num_headers);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed write HTTP headers. Error=0x%08x\n", (unsigned int) res);
        goto cleanup_disconnect;
    }

    /* Send the HTTP request and body to the server and receive the response from it. */
    res = cy_http_client_send(handle, &request, (uint8_t*) send_str, (send_str ? strlen(send_str) : 0), &client_resp);

    if (res != CY_RSLT_SUCCESS) {
        printf("Failed send the HTTP request. Error=0x%08x\n", (unsigned int) res);
        goto cleanup_disconnect;
    }

    response->data = malloc(client_resp.body_len + 1);
    if (!response->data) {
        printf("Failed to malloc response data\n");
        goto cleanup_disconnect;
    }
    memcpy(response->data, client_resp.body, client_resp.body_len);
    response->data[client_resp.body_len] = 0; // terminate the string

cleanup_disconnect:
    res = cy_http_client_disconnect(handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to disconnect the HTTP client\n");
    }

cleanup_delete:
    res = cy_http_client_delete(handle);
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to delete the HTTP client\n");
    }

cleanup_deinit:
    res = cy_http_client_deinit();
    if (res != CY_RSLT_SUCCESS) {
        printf("Failed to deinit the HTTP client\n");
    }
    return (unsigned int) res;
}

unsigned int iotconnect_https_request(IotConnectHttpResponse *response, const char *host, const char *path,
        const char *send_str) {
    return iotconnect_https_request_with_opts(response, host, path, send_str, NULL);
}

void iotconnect_free_https_response(IotConnectHttpResponse *response) {
    if (response->data) {
        free(response->data);
        response->data = NULL;
    }
}
