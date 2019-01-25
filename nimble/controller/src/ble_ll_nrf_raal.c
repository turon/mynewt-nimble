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

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include "syscfg/syscfg.h"
#include "hal/hal_timer.h"
#include "controller/ble_ll.h"
#include "controller/ble_ll_sched.h"

#if MYNEWT_VAL(BLE_LL_NRF_RAAL_ENABLE)

/*
 * Number of ticks to add at the end of scheduled item to reconfigure radio
 * after exiting from "external" slot.
 */
#define NRF_RAAL_SLOT_END_MARGIN_TICKS      (2)

/* Scheduler item for granted slot */
static struct ble_ll_sched_item g_ble_ll_nrf_raal_slot_sched;

static struct hal_timer g_ble_ll_nrf_raal_slot_end_tmr;

static int g_ble_ll_nrf_raal_continuous;

/* XXX need locking? */
static int g_ble_ll_nrf_raal_critical;

#define BLE_LL_NRF_RAAL_PENDING_SLOT_ENTER      (0x01)
#define BLE_LL_NRF_RAAL_PENDING_SLOT_EXIT       (0x02)

static int g_ble_ll_nrf_raal_pending;

extern void MYNEWT_VAL(BLE_LL_NRF_RAAL_ISR_HANDLER_NAME)(void);
extern void nrf_raal_timeslot_started(void);
extern void nrf_raal_timeslot_ended(void);

static inline bool
ble_ll_nrf_raal_slot_granted(void)
{
    return ble_ll_sched_get_current_type() == BLE_LL_SCHED_TYPE_NRF_RAAL;
}

static void
ble_ll_nrf_raal_slot_schedule(void)
{
    struct ble_ll_sched_item *sch;
    int rc;

    sch = &g_ble_ll_nrf_raal_slot_sched;

    if (!g_ble_ll_nrf_raal_continuous || sch->enqueued) {
        return;
    }

    /* Start scheduling from last slot */
    sch->start_time = sch->end_time;
    sch->end_time = sch->start_time +
                    ble_ll_usecs_to_ticks_round_up(MYNEWT_VAL(BLE_LL_NRF_RAAL_SLOT_LENGTH)) +
                    NRF_RAAL_SLOT_END_MARGIN_TICKS;

    rc = ble_ll_sched_nrf_raal(&g_ble_ll_nrf_raal_slot_sched);
    assert(rc == 0);
}

static void
ble_ll_nrf_raal_slot_enter(void)
{
    if (!g_ble_ll_nrf_raal_critical) {
        nrf_raal_timeslot_started();
        return;
    }

    /*
     * No need to make enter pending if there was exit pending since we are
     * still in slot (so basically we are still in slot).
     */
    if (g_ble_ll_nrf_raal_pending & BLE_LL_NRF_RAAL_PENDING_SLOT_EXIT) {
        /*
         * XXX is this legal? it means client was not notified that it was out
         * of slot for some time... not yet sure what this means.
         */
        g_ble_ll_nrf_raal_pending = 0;
    } else {
        g_ble_ll_nrf_raal_pending |= BLE_LL_NRF_RAAL_PENDING_SLOT_ENTER;
    }
}

static void
ble_ll_nrf_raal_slot_exit(void)
{
    if (!g_ble_ll_nrf_raal_critical) {
        nrf_raal_timeslot_ended();
        return;
    }

    /*
     * No need to make exit pending if there was enter pending since we are
     * outside slot (so basically slot was missed).
     */
    if (g_ble_ll_nrf_raal_pending & BLE_LL_NRF_RAAL_PENDING_SLOT_ENTER) {
        g_ble_ll_nrf_raal_pending = 0;
    } else {
        g_ble_ll_nrf_raal_pending |= BLE_LL_NRF_RAAL_PENDING_SLOT_EXIT;
    }
}

static void
ble_ll_nrf_raal_slot_end_tmr_cb(void *arg)
{
    ble_ll_nrf_raal_slot_exit();
    ble_phy_nrf_raal_slot_exit();
    /* XXX fixme :) */
//    ble_phy_rfclk_disable();

    /* Schedule next slot */
    ble_ll_nrf_raal_slot_schedule();

//    ble_phy_rfclk_enable();
}

static int
ble_ll_nrf_raal_slot_sched_cb(struct ble_ll_sched_item *sch)
{
    os_cputime_timer_stop(&g_ble_ll_nrf_raal_slot_end_tmr);
    os_cputime_timer_start(&g_ble_ll_nrf_raal_slot_end_tmr,
                           sch->end_time - NRF_RAAL_SLOT_END_MARGIN_TICKS);

    ble_phy_rfclk_enable();
    ble_phy_nrf_raal_slot_enter();
    ble_ll_nrf_raal_slot_enter();

    return BLE_LL_SCHED_STATE_RUNNING;
}

void
nrf_raal_init(void)
{
    /* XXX nothing to do? */
}

void
nrf_raal_uninit(void)
{
    /* XXX nothing to do? */
}

bool
nrf_raal_timeslot_is_granted(void)
{
    return ble_ll_nrf_raal_slot_granted();
}

void
nrf_raal_continuous_mode_enter(void)
{
    g_ble_ll_nrf_raal_continuous = 1;

    ble_ll_nrf_raal_slot_schedule();
}

void
nrf_raal_continuous_mode_exit(void)
{
    g_ble_ll_nrf_raal_continuous = 0;

    //if (nrf_raal_timeslot_is_granted()) {
    //    nrf_raal_timeslot_ended();
    //}
}

bool
nrf_raal_timeslot_request(uint32_t length_us)
{
    uint32_t end;

    if (!g_ble_ll_nrf_raal_continuous || !ble_ll_nrf_raal_slot_granted()) {
        return false;
    }

    end = os_cputime_get32() + ble_ll_usecs_to_ticks_round_up(length_us);

    return (int32_t)(end - ble_ll_sched_get_current_end_time()) < 0;
}


uint32_t
nrf_raal_timeslot_us_left_get(void)
{
    int32_t left;

    if (!g_ble_ll_nrf_raal_continuous || !ble_ll_nrf_raal_slot_granted()) {
        return 0;
    }

    left = (int32_t)(ble_ll_sched_get_current_end_time() - os_cputime_get32());
    if (left < 0) {
        return 0;
    }

    return os_cputime_ticks_to_usecs(left);
}

void
nrf_raal_critical_section_enter(void)
{
    g_ble_ll_nrf_raal_critical = 1;
}

void
nrf_raal_critical_section_exit(void)
{
    g_ble_ll_nrf_raal_critical = 0;

    if (g_ble_ll_nrf_raal_pending & BLE_LL_NRF_RAAL_PENDING_SLOT_ENTER) {
        ble_ll_nrf_raal_slot_enter();
    }
    if (g_ble_ll_nrf_raal_pending & BLE_LL_NRF_RAAL_PENDING_SLOT_EXIT) {
        ble_ll_nrf_raal_slot_exit();
    }

    g_ble_ll_nrf_raal_pending = 0;
}

void
ble_ll_nrf_raal_radio_isr(void)
{
    /* XXX should we check if we're in slot? PHY should not call it outside slot... */
    /* XXX any better idea how to make handler name configurable? or just always call nrf_820154 handler? */
    MYNEWT_VAL(BLE_LL_NRF_RAAL_ISR_HANDLER_NAME)();
}

void
ble_ll_nrf_raal_init(void)
{
    os_cputime_timer_init(&g_ble_ll_nrf_raal_slot_end_tmr,
                          ble_ll_nrf_raal_slot_end_tmr_cb, NULL);

    g_ble_ll_nrf_raal_slot_sched.sched_type = BLE_LL_SCHED_TYPE_NRF_RAAL;
    g_ble_ll_nrf_raal_slot_sched.sched_cb = ble_ll_nrf_raal_slot_sched_cb;

    /* We'll start scheduling form here */
    g_ble_ll_nrf_raal_slot_sched.end_time = os_cputime_get32();
}

#endif
