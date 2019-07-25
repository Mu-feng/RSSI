#pragma once

#include "main.h"
#include <stdbool.h>

void wifi_init(void);
void send_tcp_packet(uint8_t socket,uint8_t *data,uint16_t size);
bool get_tcp_data(uint8_t *socket,uint8_t *data);

