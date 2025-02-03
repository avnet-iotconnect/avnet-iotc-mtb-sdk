/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com>
 */

#include <stdio.h>
#include "cy_syslib.h"
#include "x509_crt.h"

/* mbedTLS includes. */
#include "mbedtls/pk.h"
#include "mbedtls/x509_crt.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/entropy.h"
#include "mbedtls/sha256.h"
#include "mbedtls/platform.h"
#include "mbedtls/threading.h"

#ifndef IOTC_GENCRT_KEY_CURVE
#define IOTC_GENCRT_KEY_CURVE 			MBEDTLS_ECP_DP_SECP256R1
#endif

#ifndef IOTC_GENCRT_NOT_BEFORE
#define IOTC_GENCRT_NOT_BEFORE          "20240101000000"
#endif

#ifndef IOTC_GENCRT_NOT_AFTER
#define IOTC_GENCRT_NOT_AFTER           "21240101000000"
#endif

#ifndef IOTC_GENCRT_SIGN_ALG
#define IOTC_GENCRT_SIGN_ALG			MBEDTLS_MD_SHA256
#endif

#ifndef IOTC_GENCRT_SERIAL_FUNC
#define IOTC_GENCRT_SERIAL_FUNC			Cy_SysLib_GetUniqueId
#endif

#ifndef IOTC_GENCRT_SUBJECT_NAME
#define IOTC_GENCRT_SUBJECT_NAME 		"CN=IoTConnectDevCert,O=Avnet,C=US"
#endif

// reference code from https://github.com/Mbed-TLS/mbedtls/issues/7050 and mbedtsl examples
static int generate_selfsigned_cert(mbedtls_pk_context *key, unsigned char* pem_buffer, size_t buf_len) {
	int ret = 1;
	mbedtls_mpi serial;
	mbedtls_x509write_cert crt;
	mbedtls_ctr_drbg_context ctr_drbg;

	mbedtls_mpi_init(&serial);
	mbedtls_ctr_drbg_init(&ctr_drbg);
	mbedtls_x509write_crt_init(&crt);

	// use the unique ID of chip as certificate serial number
    uint64_t hwuid = IOTC_GENCRT_SERIAL_FUNC();

	if ((ret = mbedtls_mpi_read_binary(&serial, (const unsigned char *) &hwuid, sizeof(hwuid))) != 0) {
		printf("GENCRT: Failed to read binary from hwuid!\n");
		goto exit;
	}

	mbedtls_x509write_crt_set_subject_key(&crt, key);
	mbedtls_x509write_crt_set_issuer_key(&crt, key);

	if ((ret = mbedtls_x509write_crt_set_subject_name(&crt, IOTC_GENCRT_SUBJECT_NAME)) != 0) {
		printf("GENCRT: Failed to set subject name!\n");
		goto exit;
	}


	// This will be a root cert signed by itself
	if ((ret = mbedtls_x509write_crt_set_issuer_name(&crt, IOTC_GENCRT_SUBJECT_NAME)) != 0) {
		goto exit;
	}

	mbedtls_x509write_crt_set_version(&crt, MBEDTLS_X509_CRT_VERSION_3);
	mbedtls_x509write_crt_set_md_alg(&crt, IOTC_GENCRT_SIGN_ALG);

	ret = mbedtls_x509write_crt_set_serial(&crt, &serial);
	if (ret != 0) {
		printf("GENCERT: Failed write set cert serial!\n");
		goto exit;
	}

	ret = mbedtls_x509write_crt_set_validity(&crt, IOTC_GENCRT_NOT_BEFORE, IOTC_GENCRT_NOT_AFTER);
	if (ret != 0) {
		printf("GENCERT: Failed to set cert validity!\n");
		goto exit;
	}

	ret = mbedtls_x509write_crt_set_basic_constraints(&crt, 1, 0);
	if (ret != 0) {
		printf("GENCERT: Failed to set constraints!\n");
		goto exit;
	}

	ret = mbedtls_x509write_crt_pem(&crt, pem_buffer, buf_len, mbedtls_ctr_drbg_random, &ctr_drbg);
	if (ret != 0) {
		printf("GENCERT: Failed to convert cert to pem!\n");
		goto exit;
	}

exit:
	mbedtls_mpi_free(&serial);
	mbedtls_ctr_drbg_free(&ctr_drbg);
	mbedtls_x509write_crt_free(&crt);
	return ret;
}

static int make_key(mbedtls_pk_context* key, unsigned char* key_buffer, size_t buf_len) {
	int ret;
	mbedtls_entropy_context entropy;
	mbedtls_ctr_drbg_context ctr_drbg;

	mbedtls_entropy_init(&entropy);
	mbedtls_ctr_drbg_init(&ctr_drbg);

	ret = mbedtls_pk_setup(key, mbedtls_pk_info_from_type(MBEDTLS_PK_ECKEY));
	if (ret != 0) {
		printf("GENCERT: mbedtls_pk_setup failed!\n");
		goto exit;
	}

	ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, NULL, 0);
	if (ret != 0) {
		printf("GENCERT: mbedtls_ctr_drbg_seed failed!\n");
		goto exit;
	}

	ret = mbedtls_ecp_gen_key(IOTC_GENCRT_KEY_CURVE, mbedtls_pk_ec(*key), mbedtls_ctr_drbg_random, &ctr_drbg);
	if (ret != 0) {
		printf("GENCERT: mbedtls_ecp_gen_key failed!\n");
		goto exit;
	}
	ret = mbedtls_pk_write_key_pem(key, key_buffer, buf_len);
	if (ret != 0) {
		printf("GENCERT: Failed write key to pem!\n");
		goto exit;
	}

exit:
	mbedtls_entropy_free(&entropy);
	mbedtls_ctr_drbg_free(&ctr_drbg);

	return ret;
}

int iotc_x509_generate_credentials(char* cert_buffer, size_t cert_buffer_size, char* key_buffer, size_t key_buffer_size) {
	int ret;
	mbedtls_pk_context key;
	mbedtls_pk_init(&key);

	ret = make_key(&key, (unsigned char *) key_buffer, key_buffer_size);
	if (0 != ret) {
		return ret; // called function will print the error
	}
	ret = generate_selfsigned_cert(&key, (unsigned char *) cert_buffer, cert_buffer_size);
	if (0 != ret) {
		return ret; // called function will print the error
	}
	// You can uncomment this section for dbugging purposes or if you want to record the private key
	// printf("Generated Private Key:\n%s\n", key_buffer);
	// printf("Generated Certificate:\n%s\n", cert_buffer);
	return 0;
}

