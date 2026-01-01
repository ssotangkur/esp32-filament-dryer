#ifndef TEMP_H
#define TEMP_H

#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C"
{
#endif

// Default thermistor calibration points for 100kΩ NTC thermistor
#define AIR_TEMP_SAMPLE_1_CELSIUS 25.0f
#define AIR_TEMP_SAMPLE_1_OHMS 988000.0f

#define AIR_TEMP_SAMPLE_2_CELSIUS 50.0f
#define AIR_TEMP_SAMPLE_2_OHMS 35230.0f

#define AIR_TEMP_SAMPLE_3_CELSIUS 85.0f
#define AIR_TEMP_SAMPLE_3_OHMS 10560.0f

// Air sensor hardware configuration
#define AIR_TEMP_SERIES_RESISTOR 100000.0f  // 100kΩ series resistor
#define AIR_TEMP_ADC_VOLTAGE_REFERENCE 3.3f // 3.3V ADC reference voltage

// Heater thermistor calibration points (assuming same 100kΩ NTC thermistor for now)
#define HEATER_TEMP_SAMPLE_1_CELSIUS 25.0f
#define HEATER_TEMP_SAMPLE_1_OHMS 100600.0f

#define HEATER_TEMP_SAMPLE_2_CELSIUS 50.0f
#define HEATER_TEMP_SAMPLE_2_OHMS 35980.0f

#define HEATER_TEMP_SAMPLE_3_CELSIUS 85.0f
#define HEATER_TEMP_SAMPLE_3_OHMS 10420.0f

// Heater sensor hardware configuration
#define HEATER_TEMP_SERIES_RESISTOR 100000.0f  // 100kΩ series resistor
#define HEATER_TEMP_ADC_VOLTAGE_REFERENCE 3.3f // 3.3V ADC reference voltage

  // Temperature sensor handle (opaque type for object-oriented API)
  typedef struct temp_sensor_handle *temp_sensor_handle_t;

  // Temperature-resistance data point for Steinhart-Hart coefficient calculation
  typedef struct
  {
    float temperature_celsius; // Temperature in Celsius
    float resistance_ohms;     // Thermistor resistance in ohms
  } temperature_resistance_point_t;

  // Steinhart-Hart coefficients structure
  typedef struct
  {
    float A; // First coefficient
    float B; // Second coefficient
    float C; // Third coefficient
  } steinhart_hart_coeffs_t;

  /**
   * @brief Calculate Steinhart-Hart coefficients from three temperature-resistance data points
   * @param p1 First calibration point
   * @param p2 Second calibration point
   * @param p3 Third calibration point
   * @return Steinhart-Hart coefficients structure
   */
  steinhart_hart_coeffs_t calculate_steinhart_hart_coefficients(
      temperature_resistance_point_t p1,
      temperature_resistance_point_t p2,
      temperature_resistance_point_t p3);

  /**
   * @brief Initialize ADC and temperature sampling system
   * Creates a circular buffer in PSRAM and starts background temperature reading task
   */
  void temp_sensor_init(void);

  /**
   * @brief Get handle to the air temperature sensor
   * @return Handle to the air temperature sensor, or NULL if not initialized
   */
  temp_sensor_handle_t temp_sensor_get_air_sensor(void);

  /**
   * @brief Get handle to the heater temperature sensor
   * @return Handle to the heater temperature sensor, or NULL if not initialized
   */
  temp_sensor_handle_t temp_sensor_get_heater_sensor(void);

  /**
   * @brief Get the most recent temperature reading from a sensor
   * @param sensor Handle to the temperature sensor
   * @return Latest temperature in Celsius, or -999.0f if no samples available or invalid sensor
   */
  float temp_sensor_get_reading(temp_sensor_handle_t sensor);

  /**
   * @brief Get temperature sample at specific index from a sensor (0 = oldest, buffer_count-1 = newest)
   * @param sensor Handle to the temperature sensor
   * @param index Sample index (0 to buffer_count-1)
   * @return Temperature value, or -999.0f if invalid index or sensor
   */
  float temp_sensor_get_sample(temp_sensor_handle_t sensor, size_t index);

  /**
   * @brief Get number of stored temperature samples from a sensor
   * @param sensor Handle to the temperature sensor
   * @return Number of valid samples (0 to TEMP_BUFFER_SIZE), or 0 if invalid sensor
   */
  size_t temp_sensor_get_sample_count(temp_sensor_handle_t sensor);

  /**
   * @brief Get the most recent ADC voltage reading from a sensor (calibrated if available)
   * @param sensor Handle to the temperature sensor
   * @return Latest voltage in volts, or -999.0f if no samples available or invalid sensor
   */
  float temp_sensor_get_voltage(temp_sensor_handle_t sensor);

  /**
   * @brief Get the most recent thermistor resistance reading from a sensor
   * @param sensor Handle to the temperature sensor
   * @return Latest resistance in ohms, or -999.0f if no samples available or invalid sensor
   */
  float temp_sensor_get_resistance(temp_sensor_handle_t sensor);

  /**
   * @brief Deinitialize temperature sensor (cleanup resources)
   * Stops background task and frees PSRAM buffer
   */
  void temp_sensor_deinit(void);

#ifdef __cplusplus
}
#endif

#endif // TEMP_H
