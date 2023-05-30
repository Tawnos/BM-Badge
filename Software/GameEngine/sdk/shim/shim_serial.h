#ifndef SHIM_SERIAL_H
#define SHIM_SERIAL_H

#ifdef DC801_EMBEDDED
#include <nrf_drv_usbd.h>
#include <app_usbd.h>
#include <app_usbd_cdc_acm.h>
#include <app_usbd_core.h>
#include <app_usbd_string_desc.h>
#include <app_usbd_serial_num.h>
#include <nrf_delay.h>
#endif

#include <stdint.h>
#include <stdbool.h>
#include  "utility.h"

/**
 * @brief Enable power USB detection
 *
 * Configure if example supports USB port connection
 */
#ifndef USBD_POWER_DETECTION
#define USBD_POWER_DETECTION false
#endif

#define COMMAND_BUFFER_SIZE 1024
#define COMMAND_RESPONSE_SIZE (COMMAND_BUFFER_SIZE + 128)

 // always allow for a null termination byte
#define COMMAND_BUFFER_MAX_READ (COMMAND_BUFFER_SIZE - 1)


// UART
void uart_init();
uint32_t app_uart_put(uint8_t byte);

// USB CDC
void usb_serial_init();
bool usb_serial_is_connected();
bool usb_serial_write(const char* data, uint32_t len);
bool usb_serial_read_line(char* input_buffer, uint32_t max_len);

void handle_usb_serial_input();
void send_serial_message(const char* message);

// External implementation, SDL side will talk to this
void usb_serial_connect();
uint32_t usb_serial_write_in(const char* buffer);

static bool was_serial_started{false};
static char command_buffer[COMMAND_BUFFER_SIZE];
static uint16_t command_buffer_length{COMMAND_BUFFER_SIZE};
#endif
