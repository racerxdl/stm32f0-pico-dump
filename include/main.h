/*
 * Copyright (C) 2017 Obermaier Johannes
 * Copyright (C) 2022 Lucas Teske
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#pragma once

#include <stdint.h>

#include "Arduino.h"

#define BUTTON1_Pin 4
#define LED1_Pin 25
#define TARGET_RESET_Pin 27
#define TARGET_PWR_Pin 26
#define SWDIO_Pin 14
#define SWCLK_Pin 15

#define MAX_READ_ATTEMPTS (100u)

/* all times in milliseconds */
/* minimum wait time between reset deassert and attack */
/* With my test devices this works as 0. Obermaier's default is 20 */
// #define DELAY_JITTER_MS_MIN (20u)
#define DELAY_JITTER_MS_MIN (0)
/* increment per failed attack */
#define DELAY_JITTER_MS_INCREMENT (1u)
/* maximum wait time between reset deassert and attack */
#define DELAY_JITTER_MS_MAX (50u)

void targetInit(void);

void targetReset(void);

void targetRestore(void);

void targetPowerOff(void);

void targetPowerOn(void);