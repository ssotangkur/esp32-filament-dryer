#ifndef TEMP_H
#define TEMP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * @brief Initialize ADC and temperature sampling system
   * Creates a circular buffer in PSRAM and starts background temperature reading task
   */
  void temp_sensor_init(void);

  /**
   * @brief Get the most recent temperature reading
   * @return Latest temperature in Celsius, or -999.0f if no samples available
   */
  float temp_sensor_get_reading(void);

  /**
   * @brief Get temperature sample at specific index (0 = oldest, buffer_count-1 = newest)
   * @param index Sample index (0 to buffer_count-1)
   * @return Temperature value, or -999.0f if invalid index
   */
  float temp_sensor_get_sample(size_t index);

  /**
   * @brief Get number of stored temperature samples
   * @return Number of valid samples (0 to 100)
   */
  size_t temp_sensor_get_sample_count(void);

  /**
   * @brief Get the most recent ADC voltage reading (calibrated if available)
   * @return Latest voltage in volts, or -999.0f if no samples available
   */
  float temp_sensor_get_voltage(void);

  /**
   * @brief Deinitialize temperature sensor (cleanup resources)
   * Stops background task and frees PSRAM buffer
   */
  void temp_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // TEMP_H
