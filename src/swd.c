/*
 * Copyright (C) 2017 Obermaier Johannes
 * Copyright (C) 2022 Lucas Teske
 *
 * This Source Code Form is subject to the terms of the MIT License.
 * If a copy of the MIT License was not distributed with this file,
 * you can obtain one at https://opensource.org/licenses/MIT
 */

#include "swd.h"

#include "main.h"

#define MWAIT __asm__ __volatile__( \
    ".syntax unified 		\n"          \
    "	movs r0, #0x20 		\n"          \
    "1: 	subs r0, #1 		\n"          \
    "	bne 1b 			\n"                 \
    ".syntax divided"               \
    :                               \
    :                               \
    : "cc", "r0")

#define N_READ_TURN (3u)

static uint8_t swdParity(uint8_t const* data, uint8_t const len);
static void swdDatasend(uint8_t const* data, uint8_t const len);
static void swdDataIdle(void);
static void swdDataPP(void);
static void swdTurnaround(void);
static void swdReset(void);
static void swdDataRead(uint8_t* const data, uint8_t const len);
static void swdBuildHeader(swdAccessDirection_t const adir, swdPortSelect_t const portSel, uint8_t const A32, uint8_t* const header);
static swdStatus_t swdReadPacket(swdPortSelect_t const portSel, uint8_t const A32, uint32_t* const data);
static swdStatus_t swdWritePacket(swdPortSelect_t const portSel, uint8_t const A32, uint32_t const data);
static swdStatus_t swdReadAP0(uint32_t* const data);

static uint8_t swdParity(uint8_t const* data, uint8_t const len) {
    uint8_t par = 0u;
    uint8_t cdata = 0u;
    uint8_t i;

    for (i = 0u; i < len; ++i) {
        if ((i & 0x07u) == 0u) {
            cdata = *data;
            ++data;
        }

        par ^= (cdata & 0x01u);
        cdata >>= 1u;
    }

    return par;
}

static void swdDatasend(uint8_t const* data, uint8_t const len) {
    uint8_t cdata = 0u;
    uint8_t i;

    for (i = 0u; i < len; ++i) {
        if ((i & 0x07u) == 0x00u) {
            cdata = *data;
            ++data;
        }

        if ((cdata & 0x01u) == 0x01u) {
            digitalWrite(SWDIO_Pin, HIGH);
        } else {
            digitalWrite(SWDIO_Pin, LOW);
        }
        MWAIT;

        digitalWrite(SWCLK_Pin, HIGH);
        MWAIT;
        digitalWrite(SWCLK_Pin, LOW);
        cdata >>= 1u;
        MWAIT;
    }
}

static void swdDataIdle(void) {
    digitalWrite(SWDIO_Pin, HIGH);
    MWAIT;
    pinMode(SWDIO_Pin, INPUT);
    MWAIT;
}

static void swdDataPP(void) {
    MWAIT;
    digitalWrite(SWDIO_Pin, LOW);
    pinMode(SWDIO_Pin, OUTPUT);
    MWAIT;
}

static void swdTurnaround(void) {
    digitalWrite(SWCLK_Pin, HIGH);
    MWAIT;
    digitalWrite(SWCLK_Pin, LOW);
    MWAIT;
}

static void swdDataRead(uint8_t* const data, uint8_t const len) {
    uint8_t i;
    uint8_t cdata = 0u;

    MWAIT;
    swdDataIdle();
    MWAIT;

    for (i = 0u; i < len; ++i) {
        cdata >>= 1u;
        cdata |= digitalRead(SWDIO_Pin) ? 0x80u : 0x00u;
        data[(((len + 7u) >> 3u) - (i >> 3u)) - 1u] = cdata;

        digitalWrite(SWCLK_Pin, HIGH);
        MWAIT;
        digitalWrite(SWCLK_Pin, LOW);
        MWAIT;

        /* clear buffer after reading 8 bytes */
        if ((i & 0x07u) == 0x07u) {
            cdata = 0u;
        }
    }
}

static void swdReset(void) {
    uint8_t i;

    MWAIT;
    digitalWrite(SWCLK_Pin, HIGH);
    digitalWrite(SWDIO_Pin, HIGH);
    MWAIT;

    /* 50 clk+x */
    for (i = 0u; i < (50u + 10u); ++i) {
        digitalWrite(SWCLK_Pin, HIGH);
        MWAIT;
        digitalWrite(SWCLK_Pin, LOW);
        MWAIT;
    }

    digitalWrite(SWDIO_Pin, LOW);

    for (i = 0u; i < 3u; ++i) {
        digitalWrite(SWCLK_Pin, HIGH);
        MWAIT;
        digitalWrite(SWCLK_Pin, LOW);
        MWAIT;
    }
}

static void swdBuildHeader(swdAccessDirection_t const adir, swdPortSelect_t const portSel, uint8_t const A32, uint8_t* const header) {
    if (portSel == swdPortSelectAP) {
        *header |= 0x02u; /* Access AP */
    }

    if (adir == swdAccessDirectionRead) {
        *header |= 0x04u; /* read access */
    }

    switch (A32) {
        case 0x01u:
            *header |= 0x08u;
            break;

        case 0x02u:
            *header |= 0x10u;
            break;

        case 0x03u:
            *header |= 0x18u;
            break;

        default:
        case 0x00u:

            break;
    }

    *header |= swdParity(header, 7u) << 5u;
    *header |= 0x01u; /* startbit */
    *header |= 0x80u;
}

static swdStatus_t swdReadPacket(swdPortSelect_t const portSel, uint8_t const A32, uint32_t* const data) {
    swdStatus_t ret;
    uint8_t header = 0x00u;
    uint8_t rp[1] = {0x00u};
    uint8_t resp[5] = {0u};
    uint8_t i;

    swdBuildHeader(swdAccessDirectionRead, portSel, A32, &header);

    swdDatasend(&header, 8u);
    swdDataIdle();
    swdTurnaround();
    swdDataRead(rp, 3u);

    swdDataRead(resp, 33u);

    swdDataPP();

    for (i = 0u; i < N_READ_TURN; ++i) {
        swdTurnaround();
    }

    *data = resp[4] | (resp[3] << 8u) | (resp[2] << 16u) | (resp[1] << 24u);

    ret = rp[0];

    return ret;
}

static swdStatus_t swdWritePacket(swdPortSelect_t const portSel, uint8_t const A32, uint32_t const data) {
    swdStatus_t ret;
    uint8_t header = 0x00u;
    uint8_t rp[1] = {0x00u};
    uint8_t data1[5] = {0u};
    uint8_t i;

    swdBuildHeader(swdAccessDirectionWrite, portSel, A32, &header);

    swdDatasend(&header, 8u);
    MWAIT;

    swdDataIdle();
    MWAIT;

    swdTurnaround();

    swdDataRead(rp, 3u);

    swdDataIdle();

    swdTurnaround();
    swdDataPP();

    data1[0] = data & 0xFFu;
    data1[1] = (data >> 8u) & 0xFFu;
    data1[2] = (data >> 16u) & 0xFFu;
    data1[3] = (data >> 24u) & 0xFFu;
    data1[4] = swdParity(data1, 8u * 4u);

    swdDatasend(data1, 33u);

    swdDataPP();

    for (i = 0u; i < 20u; ++i) {
        swdTurnaround();
    }

    ret = rp[0];

    return ret;
}

swdStatus_t swdReadIdcode(uint32_t* const idCode) {
    uint32_t ret;

    ret = swdReadPacket(swdPortSelectDP, 0x00u, idCode);

    return ret;
}

swdStatus_t swdSelectAPnBank(uint8_t const ap, uint8_t const bank) {
    swdStatus_t ret = swdStatusNone;
    uint32_t data = 0x00000000u;

    data |= (uint32_t)(ap & 0xFFu) << 24u;
    data |= (uint32_t)(bank & 0x0Fu) << 0u;

    /* write to select register */
    ret |= swdWritePacket(swdPortSelectDP, 0x02u, data);

    return ret;
}

static swdStatus_t swdReadAP0(uint32_t* const data) {
    swdStatus_t ret = swdStatusNone;

    swdReadPacket(swdPortSelectAP, 0x00u, data);

    return ret;
}

swdStatus_t swdSetAP32BitMode(uint32_t* const data) {
    swdStatus_t ret = swdStatusNone;

    swdSelectAPnBank(0x00u, 0x00u);

    uint32_t d = 0u;

    ret |= swdReadAP0(&d);

    ret |= swdReadPacket(swdPortSelectDP, 0x03u, &d);

    d &= ~(0x07u);
    d |= 0x02u;

    ret |= swdWritePacket(swdPortSelectAP, 0x00u, d);

    ret |= swdReadAP0(&d);
    ret |= swdReadPacket(swdPortSelectDP, 0x03u, &d);

    if (data != NULL) {
        *data = d;
    }

    return ret;
}

swdStatus_t swdSelectAHBAP(void) {
    swdStatus_t ret = swdSelectAPnBank(0x00u, 0x00u);

    return ret;
}

swdStatus_t swdReadAHBAddr(uint32_t const addr, uint32_t* const data) {
    swdStatus_t ret = swdStatusNone;
    uint32_t d = 0u;

    ret |= swdWritePacket(swdPortSelectAP, 0x01u, addr);

    ret |= swdReadPacket(swdPortSelectAP, 0x03u, &d);
    ret |= swdReadPacket(swdPortSelectDP, 0x03u, &d);

    *data = d;

    return ret;
}

swdStatus_t swdEnableDebugIF(void) {
    swdStatus_t ret = swdStatusNone;

    ret |= swdWritePacket(swdPortSelectDP, 0x01u, 0x50000000u);

    return ret;
}

swdStatus_t swdInit(uint32_t* const idcode) {
    swdStatus_t ret = swdStatusNone;

    swdReset();
    ret |= swdReadIdcode(idcode);

    return ret;
}
