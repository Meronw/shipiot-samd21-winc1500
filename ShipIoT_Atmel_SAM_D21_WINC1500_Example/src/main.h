/*

ShipIoT with Atmel SAMD21 Xplained Pro, WINC1500 WiFi, and I/O1 sensors

by Aaron Kondziela <aaron@aaronkondziela.com>

Copyright (c) 2015 wot.io, Inc. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

/**
 * \file
 *
 * \brief MAIN configuration.
 *
 * Copyright (c) 2015 Atmel Corporation. All rights reserved.
 *
 * \asf_license_start
 *
 * \page License
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * 3. The name of Atmel may not be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * 4. This software may only be redistributed and used in connection with an
 *    Atmel microcontroller product.
 *
 * THIS SOFTWARE IS PROVIDED BY ATMEL "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT ARE
 * EXPRESSLY AND SPECIFICALLY DISCLAIMED. IN NO EVENT SHALL ATMEL BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * \asf_license_stop
 *
 */

#ifndef MAIN_H_INCLUDED
#define MAIN_H_INCLUDED

#include "common/include/nm_common.h"
#include "driver/include/m2m_wifi.h"
#include "socket/include/socket.h"

#ifdef __cplusplus
extern "C" {
#endif


/******************* CONFIGURE THIS SECTION FOR YOUR ENVIRONMENT *********/
/** Wi-Fi Settings */
#define MAIN_WLAN_SSID                    "lab"                     /**< Your WiFi SSID */
#define MAIN_WLAN_PSK                     "SuperSecretPassword!"    /**< Your Password for WiFi */
#define MAIN_WLAN_AUTH                    M2M_WIFI_SEC_WPA_PSK      /**< Security type */

/** bip.io HTTP Endpoint Settings */
#define BIPIO_API_HOSTNAME              "aaronkondziela.api.shipiot.net"
#define BIPIO_API_SERVER_PORT           (80)
#define BIPIO_API_PATH                  "/bip/http/samd21example"
#define BIPIO_API_AUTH_HEADER           "Authorization: Basic YWFyb25rb25kemllbGE6N2IzNzMxYjc4N2JjOWJiYjc1ODY5MTI1YTU5M2I1M2I="
/******************** END OF ENVIRONMENT CONFIG SECTION ******************/

#define BIPIO_REPORT_INTERVAL           (1000)  /* milliseconds */
#define MAIN_WIFI_M2M_BUFFER_SIZE     (1460)

/** Sensor defines */
#define LIGHT_SENSOR_RESOLUTION	      4096    /* ADC_RESOLUTION_12BIT */

/** HTTP API request and data templates */
char http_request_template[] =		"POST " BIPIO_API_PATH " HTTP/1.1\r\n"
									"Host: " BIPIO_API_HOSTNAME "\r\n"
									BIPIO_API_AUTH_HEADER "\r\n"
									"Content-Type: application/json\r\n"
									"Content-Length: %d\r\n"
									"\r\n"
									"%s";

char json_data_template[] =			"{ \"temp\": %d, \"light\": %d }";

/** Debug banner to print to console on device boot */
#define DEBUG_BANNER "-- ShipIoT with Atmel SAM D21, WINC1500, and bip.io --\r\n" \
                     "-- " BOARD_NAME " --\r\n" \
                     "-- Compiled: " __DATE__ " " __TIME__ " --\r\n"
#ifdef __cplusplus
}
#endif

static void init_board(void);
static void init_wifi(void);
static void init_network(void);
static void configure_adc(void);
static void configure_console(void);
static void wifi_cb(uint8_t u8MsgType, void *pvMsg);
static void resolve_cb(uint8_t *name, uint32_t hostip);
static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg);
static uint16_t get_light_value(void);
static void send_telemetry(void);

#endif /* MAIN_H_INCLUDED */
