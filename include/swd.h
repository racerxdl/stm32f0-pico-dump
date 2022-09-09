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

/* Internal SWD status. There exist combined SWD status values (e.g. 0x60), since subsequent command replys are OR'ed. Thus there exist cases where the previous command executed correctly (returned 0x20) and the following command failed (returned 0x40), resulting in 0x60. */
typedef enum {
    // TODO: 0xA0 fehlt.
    swdStatusNone = 0x00u,    /* No status available (yet) */
    swdStatusOk = 0x20u,      /* Status OK */
    swdStatusWait = 0x40u,    /* Wait/Retry requested (bus access was not granted in time) */
    swdStatusWaitOK = 0x60u,  /* Wait requested + additional OK (previous command OK, but no bus access) */
    swdStatusFault = 0x80u,   /* Fault during command execution (command error (access denied etc.)) */
    swdStatusFaultOK = 0xA0u, /* Fault during command execution, previous command was successful */
    swdStatusFailure = 0xE0u  /* Failure during communication (check connection, no valid status reply received) */
} swdStatus_t;

typedef enum {
    swdPortSelectDP = 0x00u,
    swdPortSelectAP = 0x01u
} swdPortSelect_t;

typedef enum {
    swdAccessDirectionWrite = 0x00u,
    swdAccessDirectionRead = 0x01u
} swdAccessDirection_t;

swdStatus_t swdEnableDebugIF(void);
swdStatus_t swdReadIdcode(uint32_t* const idCode);
swdStatus_t swdSelectAPnBank(uint8_t const ap, uint8_t const bank);
swdStatus_t swdReadAHBAddr(uint32_t const addr, uint32_t* const data);
swdStatus_t swdInit(uint32_t* const idcode);
swdStatus_t swdSetAP32BitMode(uint32_t* const data);
swdStatus_t swdSelectAHBAP(void);
