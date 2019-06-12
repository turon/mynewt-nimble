/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <stdint.h>

#include "nimble/nimble_port.h"

typedef void (*irq_handler_t)(void);

static uint32_t radio_isr_addr;
static uint32_t rng_isr_addr;
static uint32_t rtc0_isr_addr;

/* Some interrupt handlers required for NimBLE radio driver */

void
RADIO_IRQHandler(void)
{
    ((irq_handler_t)radio_isr_addr)();
}

/*
void
RNG_IRQHandler(void)
{
    ((irq_handler_t)rng_isr_addr)();
}
*/

void
RTC0_IRQHandler(void)
{
    ((irq_handler_t)rtc0_isr_addr)();
}

/* This is called by NimBLE radio driver to set interrupt handlers */
void
npl_freertos_hw_set_isr(int irqn, uint32_t addr)
{
    switch (irqn) {
    case RADIO_IRQn:
        radio_isr_addr = addr;
        break;
    /*
    case RNG_IRQn:
        rng_isr_addr = addr;
        break;
    */
    case RTC0_IRQn:
        rtc0_isr_addr = addr;
        break;
    }
}
