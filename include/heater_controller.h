#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "freertos/FreeRTOS.h" // Include FreeRTOS for SemaphoreHandle_t

/** @brief Configuration parameters for the heater controller. */
typedef struct
{
    float max_heater_temp;        // Max safe temperature for the heater element.
    float air_temp_hysteresis;    // Hysteresis for air temp bang-bang control (e.g., 1.0f).
    float heater_temp_hysteresis; // Hysteresis for heater temp bang-bang control (e.g., 2.0f).
    float full_power_delta;       // Temp delta below target to engage full power mode (e.g. 5.0f).
} controller_config_t;

// Access to internal state for testing purposes only
typedef enum
{
    CONTROLLER_STATE_IDLE,
    CONTROLLER_STATE_HEATING_FULL_POWER,
    CONTROLLER_STATE_MODULATING_HEATER_TEMP,
    CONTROLLER_STATE_MAINTAINING_AIR_TEMP,
} controller_state_t;

typedef struct
{
    controller_config_t config;
    float target_temp;
    bool active;
    controller_state_t state;
    SemaphoreHandle_t mutex;
    bool initialized;                   // Flag to indicate if controller is initialized
    bool heater_safety_override_active; // Flag to indicate if safety override is active
    uint8_t current_power;              // Current power level commanded to the heater
} controller_internal_state_t;

/**
 * @brief Provides a pointer to the internal state of the controller for testing or debugging.
 * @return Pointer to `controller_internal_state_t`.
 */
controller_internal_state_t *controller_get_state(void);

/**
 * @brief Initializes the controller and its internal mutex.
 * @param config Pointer to a struct with the controller's operating parameters.
 * @param initial_target_temp The initial target air temperature.
 */
void controller_init(const controller_config_t *config, float initial_target_temp);

/** @brief Deletes the controller's mutex. */
void controller_deinit(void);

/**
 * @brief Sets a new target air temperature in a thread-safe manner.
 * @param temp The desired target air temperature.
 */
void controller_set_target_temp(float temp);

/**
 * @brief Activates or deactivates the heater control logic.
 *        If deactivated, the heater will be forced off (IDLE state)
 *        regardless of target or current temperatures.
 * @param active True to activate control, false to deactivate and force IDLE.
 */
void controller_set_active(bool active);

/**
 * @brief Executes one cycle of the control loop.
 * @note This function is designed to be called periodically by a high-level task.
 * @param heater_temp The current temperature of the heater element.
 * @param air_temp The current air temperature.
 */
void controller_run(float heater_temp, float air_temp);
