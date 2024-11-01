// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

#define UART_RX_BUFFER_SIZE 1024

void uart_listen();
void uart_listen_char_limited();

void uart_rx_buffer_init();
bool uart_rx_buffer_is_empty();
bool uart_rx_buffer_is_full();
bool uart_rx_buffer_put(uint8_t byte);
uint8_t uart_rx_buffer_get();
void uart_rx_irq_callback();

typedef struct __packed _LlamaMessage {
    uint8_t command[8];
    uint8_t payload[32];
} LlamaMessage;
