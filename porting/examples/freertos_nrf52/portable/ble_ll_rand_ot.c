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
#include "nimble/ble.h"

/**
 * Initialize LL random number generation. Should be called only once on
 * initialization.
 *
 * @return int
 */
int
ble_ll_rand_init(void)
{
    return 0;
}

/**
 * Start the generation of random numbers
 *
 * @return int
 */
int
ble_ll_rand_start(void)
{
    return 0;
}

/* Get 'len' bytes of random data */
int
ble_ll_rand_data_get(uint8_t *buf, uint8_t len)
{
    return otPlatRandomGetTrue(buf, len) ? BLE_ERR_UNSPECIFIED : BLE_ERR_SUCCESS;
}

/**
 * Called to obtain a "prand" as defined in core V4.2 Vol 6 Part B 1.3.2.2
 *
 * @param prand
 */
void
ble_ll_rand_prand_get(uint8_t *prand)
{
    uint16_t sum;

    while (1) {
        /* Get 24 bits of random data */
        ble_ll_rand_data_get(prand, 3);

        /* Prand cannot be all zeros or 1's. */
        sum = prand[0] + prand[1] + prand[2];
        if ((sum != 0) && (sum != (3 * 0xff))) {
            break;
        }
    }

    /* Upper two bits must be 01 */
    prand[2] &= ~0xc0;
    prand[2] |= 0x40;
}
