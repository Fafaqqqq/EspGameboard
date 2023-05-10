#ifndef WIFI_INIT_H
#define WIFI_INIT_H

#include <stdint.h>

void wifi_init();

int32_t wifi_set_tx(const uint8_t* data, uint32_t size);
int32_t wifi_get_rx(uint8_t* data, uint32_t* size);

#endif // WIFI_INIT_H