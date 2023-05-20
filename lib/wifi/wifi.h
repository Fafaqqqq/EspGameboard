#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include <stdint.h>

void wifi_init();

void wifi_init_ap
(
  void *pvParams
);

void wifi_init_sta
(
  void *pvParams
);

const char** wifi_scan_esp_net
(
  uint32_t* net_cnt
);

int32_t wifi_set_tx
(
  const void* data,
  uint32_t size
);

int32_t wifi_get_rx
(
  void* data,
  uint32_t* size
);

#endif // WIFI_INIT_H