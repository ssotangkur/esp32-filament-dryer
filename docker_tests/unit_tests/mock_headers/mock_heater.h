#ifndef MOCK_HEATER_H
#define MOCK_HEATER_H

#include <stdint.h>

void heater_init(void);
void set_heat_power(uint8_t power);

#endif // MOCK_HEATER_H