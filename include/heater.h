#pragma once

#include <stdint.h>

/** @brief Initializes the heater hardware peripherals. */
void heater_init(void);

/**
 * @brief Sets the heater power level.
 * @param power Power level from 0 (off) to 255 (max).
 */
void set_heat_power(uint8_t power);