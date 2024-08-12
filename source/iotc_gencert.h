/* SPDX-License-Identifier: MIT
 * Copyright (C) 2024 Avnet
 * Authors: Nikola Markovic <nikola.markovic@avnet.com>
 */

#pragma once

#include <stdlib.h>

int iotc_x509_generate_credentials(
		char* pem_certificate_buffer,
		size_t pem_certificate_buffer_size,
		char* pem_private_key_buffer,
		size_t pem_private_key_buffer_size
);
