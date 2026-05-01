/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com> et al.
 */

#ifndef IOTC_HTTP_CLIENT_H
#define IOTC_HTTP_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IOTC_HTTP_HEADERS_MAX
#define IOTC_HTTP_HEADERS_MAX 6 // Max supported headers. Re-define if needed.
#endif

typedef struct {
    char *name;
    char *value;
} IotConnectHttpHeader;

typedef struct {
    char *ca_cert; // if empty, GoDaddy G2 will be used.
    char *cert; // Optional: Used for mTLS.
    char *key; // Optional: Used for mTLS.
    IotConnectHttpHeader *headers; // Optional: Additional headers to include in the request (array).
    size_t headers_len; // Number of additional headers . Ignored if headers is NULL.
} IotConnectHttpOpts;

typedef struct IotConnectHttpResponse {
    char *data; // add flexibility for future, but at this point we only have response data
} IotConnectHttpResponse;

// Helper to deal with http chunked transfers which are always returned by iotconnect services.
// Free data with iotconnect_free_https_response
unsigned int iotconnect_https_request(
        IotConnectHttpResponse* response,
        const char *host,
        const char *path,
        const char *send_str
);

unsigned int iotconnect_https_request_with_opts(
        IotConnectHttpResponse* response,
        const char *host,
        const char *path,
        const char *send_str,
        IotConnectHttpOpts* opts // if NULL, same as iotconnect_https_request()
);


void iotconnect_free_https_response(IotConnectHttpResponse* response);

#ifdef __cplusplus
}
#endif

#endif //IOTC_HTTP_CLIENT_H
