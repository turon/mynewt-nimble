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

#include <stdbool.h>
#include <stdint.h>

#include <pthread.h>
#include "nimble/nimble_npl.h"
#include "nimble/nimble_port.h"

static struct ble_npl_task s_task_host;
static struct ble_npl_task s_task_hci;

void nimble_host_task(void *param);
void ble_hci_sock_ack_handler(void *param);
void ble_hci_sock_init(void);

#define TASK_DEFAULT_PRIORITY       1
#define TASK_DEFAULT_STACK          NULL
#define TASK_DEFAULT_STACK_SIZE     400

void *ble_hci_sock_task(void *param)
{
    ble_hci_sock_ack_handler(param);
    return NULL;
}

void *ble_host_task(void *param)
{
    nimble_host_task(param);
    return NULL;
}

int main(void)
{
    ble_hci_sock_init();
    nimble_port_init();

    ble_npl_task_init(&s_task_hci, "hci_sock", ble_hci_sock_task,
                      NULL, TASK_DEFAULT_PRIORITY, BLE_NPL_WAIT_FOREVER,
                      TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);

    /* Create task which handles default event queue */
    ble_npl_task_init(&s_task_host, "ble_host", ble_host_task,
                      NULL, TASK_DEFAULT_PRIORITY, BLE_NPL_WAIT_FOREVER, 
                      TASK_DEFAULT_STACK, TASK_DEFAULT_STACK_SIZE);

    int ret = 0;
    pthread_exit(&ret);         
         
    while (true)
    {
        pthread_yield();
    }
}
