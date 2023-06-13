#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include <stdint.h>

typedef int socket_t;

socket_t wifi_accept();
socket_t wifi_connect();

int32_t wifi_transmit(socket_t sock, const void* local, int32_t size);
int32_t wifi_recieve(socket_t sock, void* local, int32_t* size, uint32_t max_size);

void wifi_init();

int wifi_init_ap
(
  void *pvParams
);

int wifi_init_sta
(
  void *pvParams
);

const char** wifi_scan_esp_net
(
  uint32_t* net_cnt
);

int32_t wifi_start_tcp
(
  uint32_t tx_size,
  uint32_t rx_size,
  int      tcp_type
);

int32_t wifi_stop_tcp
(
  uint32_t tx_size,
  uint32_t rx_size
);

int32_t wifi_set_tx
(
  const void* data
);

int32_t wifi_get_rx
(
  void* data
);

#endif // WIFI_INIT_H