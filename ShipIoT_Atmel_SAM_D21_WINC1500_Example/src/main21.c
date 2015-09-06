/*

ShipIoT with Atmel SAMD21 Xplained Pro, WINC1500 WiFi, and I/O1 sensors
Based on Atmel's "WINC1500 Simple TCP Client Example for SAM D21" code

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
 
/*
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

/** \mainpage
 * \section intro Introduction
 *
 * ShipIoT with Atmel SAMD21 Xplained Pro, WINC1500 WiFi, and I/O1 sensors
 *
 * Based on Atmel's "WINC1500 Simple TCP Client Example for SAM D21" code
 *
 * It uses the following hardware:
 * - the SAMD21 Xplained Pro.
 * - the WINC1500 connected on port EXT1.
 * - the I/O1 Xplained connected on port EXT2
 *
 * \section files Main Files
 * - main.c : Initialize the WINC1500 and test TCP client.
 *
 * \section usage Usage
 * -# Configure the defines in main.h for WiFi and the bip.io HTTP endpoint.
 * -# Build the program and download it into the board.
 * -# On the computer, open and configure a terminal application as the follows.
 * \code
 *    Baud Rate : 115200
 *    Data : 8bit
 *    Parity bit : none
 *    Stop bit : 1bit
 *    Flow control : none
 * \endcode
 * -# Start the application.
 * -# In the terminal window, you will see debug output describing the operation of the application.
 * -# Watch your ShipIoT workflow output  while you vary the input to the light and temperature sensors.
 *
 * \section compinfo Compilation Information
 * This software was written for the GNU GCC compiler using Atmel Studio 6.2
 * Other compilers may or may not work.
 *
 * \section contactinfo Contact Information
 * For further information, visit
 * <A href="http://www.atmel.com">Atmel</A> and <a href="http://shipiot.net">ShipIoT</a>.\n
 */

#include "asf.h"
#include "main.h"

/** UART module for debug. */
static struct usart_module cdc_uart_module;

/** Receive buffer definition. */
static uint8_t gau8SocketTestBuffer[MAIN_WIFI_M2M_BUFFER_SIZE];

/** Socket for client */
static SOCKET tcp_client_socket = -1;

/** Wi-Fi connection state */
static uint8_t wifi_connected;

/** Address for our client socket */
struct sockaddr_in addr;

/* Light sensor module. */
struct adc_module adc_instance;


static void wifi_cb(uint8_t u8MsgType, void *pvMsg)
{
	tstrM2mWifiStateChanged *pstrWifiState;
	uint8_t *pu8IPAddress;
	
	switch (u8MsgType) {
		case M2M_WIFI_RESP_CON_STATE_CHANGED:
		pstrWifiState = (tstrM2mWifiStateChanged *)pvMsg;
		if (pstrWifiState->u8CurrState == M2M_WIFI_CONNECTED) {
			printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: CONNECTED\r\n");
			m2m_wifi_request_dhcp_client();
			} else if (pstrWifiState->u8CurrState == M2M_WIFI_DISCONNECTED) {
			printf("wifi_cb: M2M_WIFI_RESP_CON_STATE_CHANGED: DISCONNECTED\r\n");
			wifi_connected = 0;
			m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
		}
		break;

		case M2M_WIFI_REQ_DHCP_CONF:
		pu8IPAddress = (uint8_t *)pvMsg;
		wifi_connected = 1;
		printf("wifi_cb: M2M_WIFI_REQ_DHCP_CONF: IP is %u.%u.%u.%u\r\n",
		pu8IPAddress[0], pu8IPAddress[1], pu8IPAddress[2], pu8IPAddress[3]);
		break;

		default:
		break;
	}
}


static void resolve_cb(uint8_t *name, uint32_t hostip)
{
	printf("Received DNS response, setting IP address\r\n");
	addr.sin_addr.s_addr = hostip;
}


static void socket_cb(SOCKET sock, uint8_t u8Msg, void *pvMsg)
{
	tstrSocketConnectMsg *pstrConnect;
	tstrSocketRecvMsg *pstrRecv;
	
	switch (u8Msg) {
		/* Socket connected */
		case SOCKET_MSG_CONNECT:
			pstrConnect = (tstrSocketConnectMsg *)pvMsg;
			if (pstrConnect && pstrConnect->s8Error >= 0) {
				printf("socket_cb: connect success!\r\n" );
				send_telemetry();
			} else {
				printf("socket_cb: connect error!\r\n" );
				close(tcp_client_socket);
				tcp_client_socket = -1;
			}
			break;

		/* Message send */
		case SOCKET_MSG_SEND:
			printf("socket_cb: send success!\r\n");
			recv(tcp_client_socket, gau8SocketTestBuffer, sizeof(gau8SocketTestBuffer), 0);
			break;

		/* Message receive */
		case SOCKET_MSG_RECV:
			pstrRecv = (tstrSocketRecvMsg *)pvMsg;
			if (pstrRecv && pstrRecv->s16BufferSize > 0) {
				printf("socket_cb: recv success!\r\n");
				close(tcp_client_socket);
				tcp_client_socket = -1;
			} else {
				printf("socket_cb: recv error!\r\n");
				close(tcp_client_socket);
				tcp_client_socket = -1;
			}
			break;
			
		default:
			break;
	}
}


static uint16_t get_light_value(void)
{
	uint16_t light_val;
	status_code_genare_t adc_status;
	adc_start_conversion(&adc_instance);
	do {
		adc_status = adc_read(&adc_instance, &light_val);
	} while (adc_status == STATUS_BUSY);
	return light_val;
}


static void send_telemetry(void)
{
	// Adjust for temperature rise of board due to component power dissipation
	// You would want to measure this constant in a real application
	double tempc = at30tse_read_temperature() - 1.0;
	
	// Fetch the light sensor value, and normalize it to a 0-100 range
	uint8_t light = 100 - (get_light_value() * 100 / LIGHT_SENSOR_RESOLUTION);
	
	printf("Sending telemetry data... %.1f and %d\r\n", tempc, light);
	
	// Populate our telemetry json message
	char telemetry_buffer[255];
	sprintf(telemetry_buffer, json_data_template, (uint16_t)tempc, light);
	
	// Populate our http request template
	char http_request[1024];
	sprintf(http_request, http_request_template, strlen(telemetry_buffer), telemetry_buffer);
	
	// And send it to the bip workflow endpoint
	send(tcp_client_socket, (uint8 *)http_request, sizeof(http_request), 0);
}


static void configure_console(void)
{
	struct usart_config usart_conf;

	usart_get_config_defaults(&usart_conf);
	usart_conf.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	usart_conf.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	usart_conf.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	usart_conf.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	usart_conf.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	usart_conf.baudrate    = 115200;

	stdio_serial_init(&cdc_uart_module, EDBG_CDC_MODULE, &usart_conf);
	usart_enable(&cdc_uart_module);
}


static void configure_adc(void)
{
	struct adc_config config_adc;
	adc_get_config_defaults(&config_adc);
	config_adc.gain_factor     = ADC_GAIN_FACTOR_DIV2;
	config_adc.clock_prescaler = ADC_CLOCK_PRESCALER_DIV32;
	config_adc.reference       = ADC_REFERENCE_INTVCC1;
	config_adc.positive_input  = ADC_POSITIVE_INPUT_PIN18;
	config_adc.resolution      = ADC_RESOLUTION_12BIT;
	config_adc.freerunning     = true;
	config_adc.left_adjust     = false;
	adc_init(&adc_instance, ADC, &config_adc);
	adc_enable(&adc_instance);
}


static void init_board(void)
{
	system_init();
	nm_bsp_init();

	configure_console();
	printf(DEBUG_BANNER);

	at30tse_init();       // Temperature sensor
	configure_adc();      // Configure the ADC for the light sensor
}


static void init_wifi(void)
{
	tstrWifiInitParam param;
	int8_t ret;

	/* Initialize Wi-Fi parameters structure. */
	memset((uint8_t *)&param, 0, sizeof(tstrWifiInitParam));

	/* Initialize Wi-Fi driver with data and status callbacks. */
	param.pfAppWifiCb = wifi_cb;
	ret = m2m_wifi_init(&param);
	if (M2M_SUCCESS != ret) {
		printf("main: m2m_wifi_init call error, halting!(%d)\r\n", ret);
		while (1) {
		}
	}	
}


static void init_network(void)
{
	/* Initialize global socket address structure. */
	addr.sin_family = AF_INET;
	addr.sin_port = _htons(BIPIO_API_SERVER_PORT);
	addr.sin_addr.s_addr = 0;    // 0 flags this to be resolved by DNS later on
		
	/* Initialize socket module and register socket callbacks */
	socketInit();
	registerSocketCallback(socket_cb, resolve_cb);

	/* Connect to router. */
	m2m_wifi_connect((char *)MAIN_WLAN_SSID, sizeof(MAIN_WLAN_SSID), MAIN_WLAN_AUTH, (char *)MAIN_WLAN_PSK, M2M_WIFI_CH_ALL);
}


int main(void)
{
	uint16_t loops = 0;

	init_board();
	init_wifi();
	init_network();

	while (1) {
		loops++;
		/* Handle pending events from network controller. */
		m2m_wifi_handle_events(NULL);
		
		if ( (loops % BIPIO_REPORT_INTERVAL == 0) && wifi_connected == M2M_WIFI_CONNECTED) {
			loops = 0;
			printf("Sending...\r\n");
			
			// Resolve server hostname if we haven't done it yet.
			if (addr.sin_addr.s_addr == 0) {
				printf("Resolving hostname...\r\n");
				gethostbyname((uint8_t *)BIPIO_API_HOSTNAME);
			}
			
			/* Open client socket if it is closed, only after the name is resolved. */
			if (tcp_client_socket < 0 && addr.sin_addr.s_addr != 0) {
				if ((tcp_client_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
					printf("Error: failed to create TCP client socket\r\n");
					continue;
				}

				/* Connect server, and the socket callback will send our telemetry upon connection */
				printf("Connecting...\r\n");
				if (connect(tcp_client_socket, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) < 0) {
					printf("Error: connect failed, closing socket.\r\n");
					close(tcp_client_socket);
					tcp_client_socket = -1;
				}
			}
		}
		
		nm_bsp_sleep(1);
	}

	return 0;
}
