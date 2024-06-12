// SPDX-License-Identifier: GPL-2.0-only
// Copyright (C) 2022, Input Labs Oy.

#pragma once

// BOARD PINS
#define PIN_GROUP_BOARD 1
#define PIN_GROUP_BOARD_END 99

// Pico.
#ifdef DEVICE_ALPAKKA_V0
    #define PIN_LED_BOARD 25
    #define PIN_LED_UP 2
    #define PIN_LED_RIGHT 5
    #define PIN_LED_DOWN 4
    #define PIN_LED_LEFT 3
    #define PIN_HOME 20
    #define PIN_TOUCH_OUT 6
    #define PIN_TOUCH_IN 7
    #define PIN_ROTARY_A 9
    #define PIN_ROTARY_B 8
    #define PIN_I2C_SDA 14
    #define PIN_I2C_SCL 15
    #define PIN_SPI_CK 10
    #define PIN_SPI_TX 11
    #define PIN_SPI_RX 12
    #define PIN_SPI_CS0 18
    #define PIN_SPI_CS1 19
    #define PIN_THUMBSTICK_LX 27
    #define PIN_THUMBSTICK_LY 26
    #define SPI_CHANNEL spi1
#endif

// Custom board.
#if defined(DEVICE_ALPAKKA_V1) || defined(DEVICE_DONGLE)
    #define PIN_LED_BOARD 2
    #define PIN_LED_UP 3
    #define PIN_LED_RIGHT 4
    #define PIN_LED_DOWN 5
    #define PIN_LED_LEFT 6
    #define PIN_HOME 7
    #define PIN_TOUCH_OUT 8
    #define PIN_TOUCH_IN 9
    #define PIN_ROTARY_A 12
    #define PIN_ROTARY_B 13
    #define PIN_I2C_SDA 20
    #define PIN_I2C_SCL 21
    #define PIN_SPI_CK 18
    #define PIN_SPI_TX 19
    #define PIN_SPI_RX 16
    #define PIN_SPI_CS0 22
    #define PIN_SPI_CS1 23
    #define PIN_SPI_CS_NRF24 17
    #define PIN_THUMBSTICK_LX 27
    #define PIN_THUMBSTICK_LY 26
    #define SPI_CHANNEL spi0
#endif

// IO EXPANSION 1.
#define PIN_GROUP_IO_0 100
#define PIN_GROUP_IO_0_END 199
#define PIN_SELECT_1 114
#define PIN_SELECT_2 113
#define PIN_DPAD_LEFT 104
#define PIN_DPAD_RIGHT 101
#define PIN_DPAD_UP 103
#define PIN_DPAD_DOWN 100
#define PIN_L1 102
#define PIN_L2 115
#define PIN_L3 112
#define PIN_L4 109
#define PIN_PCBGEN_0 111
#define PIN_PCBGEN_1 110

// IO EXPANSION 2.
#define PIN_GROUP_IO_1 200
#define PIN_GROUP_IO_1_END 249
#define PIN_START_1 200
#define PIN_START_2 201
#define PIN_A 215
#define PIN_B 210
#define PIN_X 213
#define PIN_Y 211
#define PIN_DHAT_LEFT 203
#define PIN_DHAT_RIGHT 205
#define PIN_DHAT_UP 206
#define PIN_DHAT_DOWN 202
#define PIN_DHAT_PUSH 204
#define PIN_R1 212
#define PIN_R2 214
#define PIN_R4 207

// SPECIAL PINS.
#define PIN_GROUP_SPECIAL 250
#define PIN_NONE 255
#define PIN_VIRTUAL 254  // Buttons without any hardware associated to them.
