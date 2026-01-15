#ifndef MOCK_HEATER_H
#define MOCK_HEATER_H

#include "cmock.h"
#include <stdint.h>

// Mock declarations for heater_init
void heater_init_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void heater_init_CMockExpect(UNITY_LINE_TYPE cmock_line);

// Mock declarations for set_heat_power
void set_heat_power_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void set_heat_power_CMockExpect(UNITY_LINE_TYPE cmock_line, uint8_t power);

#endif // MOCK_HEATER_H
