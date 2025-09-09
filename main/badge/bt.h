#ifndef __BT_H__
#define __BT_H__

#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "common/bt_hci_common.h"

#include "esp_bt.h"
#include "badge.h"

#define BLE_SCAN_INTERVAL 0x50  // it will scan every X * 0,625ms
#define BLE_SCAN_WINDOW   0x30  // it will scan for X * 0,625ms

#define BLE_ADV_MIN 5 * 0x640  // X seconds * 0x640
#define BLE_ADV_MAX 5 * 0x640  // X seconds * 0x640

#define NODE_QUEUE_TIMEOUT_MS 20000

extern ble_node_t ble_nodes[MAX_NEARBY_NODE];

typedef struct {
  char scan_local_name[32];
  uint8_t name_len;
} ble_scan_local_name_t;

typedef struct {
  uint8_t *q_data;
  uint16_t q_data_len;
} host_rcv_data_t;

uint8_t count_ble_nodes();
bool check_ble_set();

void bt_init();
void bt_task(void *);

#endif