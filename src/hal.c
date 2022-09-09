/*
 * Copyright (C) 2017 Obermaier Johannes
 * Copyright (C) 2022 Lucas Teske
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#include "main.h"

void targetInit(void) {
    targetPowerOff();
    targetReset();
}

void targetReset(void) {
    digitalWrite(TARGET_RESET_Pin, LOW);
}

void targetRestore(void) {
    digitalWrite(TARGET_RESET_Pin, HIGH);
}

void targetPowerOff(void) {
    digitalWrite(TARGET_PWR_Pin, LOW);
}

void targetPowerOn(void) {
    digitalWrite(TARGET_PWR_Pin, HIGH);
}