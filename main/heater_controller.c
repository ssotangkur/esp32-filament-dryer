#include "heater_controller.h"
#include "heater.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include <stddef.h> // Required for NULL

static const char *TAG = "CONTROLLER";

static controller_internal_state_t s_controller_state;

void controller_init(const controller_config_t *config, float initial_target_temp)
{
    if (s_controller_state.initialized)
    {
        ESP_LOGW(TAG, "Controller already initialized.");
        return;
    }

    if (config == NULL)
    {
        ESP_LOGE(TAG, "Controller configuration is NULL!");
        return;
    }

    // Copy configuration
    s_controller_state.config = *config;
    s_controller_state.target_temp = initial_target_temp;
    s_controller_state.active = true;                         // Active by default
    s_controller_state.state = CONTROLLER_STATE_IDLE;         // Start in IDLE state
    s_controller_state.heater_safety_override_active = false; // Not active initially
    s_controller_state.current_power = 0;                     // Heater off initially

    // Create mutex for thread-safe access
    s_controller_state.mutex = xSemaphoreCreateMutex();
    if (s_controller_state.mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create controller mutex!");
        return;
    }

    s_controller_state.initialized = true;
    ESP_LOGI(TAG, "Controller initialized successfully. Max Heater Temp: %.2f, Initial Target: %.2f",
             s_controller_state.config.max_heater_temp, s_controller_state.target_temp);
}

// Function to provide access to internal state for testing purposes
controller_internal_state_t *controller_get_state(void)
{
    return &s_controller_state;
}

void controller_deinit(void)
{
    if (!s_controller_state.initialized)
    {
        ESP_LOGW(TAG, "Controller not initialized, cannot deinitialize.");
        return;
    }

    if (s_controller_state.mutex != NULL)
    {
        vSemaphoreDelete(s_controller_state.mutex);
        s_controller_state.mutex = NULL;
    }
    s_controller_state.initialized = false;
    ESP_LOGI(TAG, "Controller deinitialized.");
}

void controller_set_target_temp(float temp)
{
    if (!s_controller_state.initialized)
    {
        ESP_LOGW(TAG, "Controller not initialized, cannot set target temp.");
        return;
    }
    if (xSemaphoreTake(s_controller_state.mutex, portMAX_DELAY) == pdTRUE)
    {
        s_controller_state.target_temp = temp;
        ESP_LOGD(TAG, "Target temperature set to %.2f", temp);
        xSemaphoreGive(s_controller_state.mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex to set target temp.");
    }
}

void controller_set_active(bool active)
{
    if (!s_controller_state.initialized)
    {
        ESP_LOGW(TAG, "Controller not initialized, cannot set active state.");
        return;
    }
    if (xSemaphoreTake(s_controller_state.mutex, portMAX_DELAY) == pdTRUE)
    {
        s_controller_state.active = active;
        ESP_LOGI(TAG, "Controller set to %s", active ? "ACTIVE" : "INACTIVE");
        xSemaphoreGive(s_controller_state.mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex to set active state.");
    }
}

void controller_run(float heater_temp, float air_temp)
{
    if (!s_controller_state.initialized)
    {
        ESP_LOGW(TAG, "Controller not initialized, cannot run.");
        return;
    }

    if (xSemaphoreTake(s_controller_state.mutex, portMAX_DELAY) == pdTRUE)
    {
        // Global Control State: If deactivated, force IDLE
        if (!s_controller_state.active)
        {
            if (s_controller_state.state != CONTROLLER_STATE_IDLE)
            {
                ESP_LOGI(TAG, "Controller deactivated, forcing IDLE state.");
                s_controller_state.state = CONTROLLER_STATE_IDLE;
                set_heat_power(0);
            }
            xSemaphoreGive(s_controller_state.mutex);
            return; // Exit if not active
        }

        // Global Safety Override: Check heater_temp regardless of state
        if (heater_temp >= s_controller_state.config.max_heater_temp)
        {
            if (s_controller_state.state != CONTROLLER_STATE_IDLE)
            {
                ESP_LOGW(TAG, "Heater temp %.2fC >= Max Heater Temp %.2fC. Forcing IDLE (safety override).",
                         heater_temp, s_controller_state.config.max_heater_temp);
                s_controller_state.state = CONTROLLER_STATE_IDLE;
                set_heat_power(0);
            }
            s_controller_state.heater_safety_override_active = true;
            xSemaphoreGive(s_controller_state.mutex);
            return; // Exit if safety override is active
        }

        // Only allow heater to come out of IDLE (due to safety override) once temp has dropped sufficiently
        if (s_controller_state.state == CONTROLLER_STATE_IDLE &&
            s_controller_state.heater_safety_override_active &&
            heater_temp > (s_controller_state.config.max_heater_temp - s_controller_state.config.heater_temp_hysteresis))
        {
            ESP_LOGD(TAG, "Heater still too hot (%.2fC) to exit safety override. Remaining IDLE.", heater_temp);
            xSemaphoreGive(s_controller_state.mutex);
            return;
        }
        else if (s_controller_state.heater_safety_override_active &&
                 heater_temp <= (s_controller_state.config.max_heater_temp - s_controller_state.config.heater_temp_hysteresis))
        {
            ESP_LOGI(TAG, "Heater temp %.2fC below safety threshold. Exiting safety override.", heater_temp);
            s_controller_state.heater_safety_override_active = false;
        }

        // State Machine Logic
        switch (s_controller_state.state)
        {
        case CONTROLLER_STATE_IDLE:
            // Transition out of IDLE if conditions met
            if (air_temp < (s_controller_state.target_temp - s_controller_state.config.full_power_delta) &&
                !s_controller_state.heater_safety_override_active)
            {
                ESP_LOGI(TAG, "AIR Temp %.2fC < Target %.2fC - Delta %.2fC. Transitioning to HEATING_FULL_POWER.",
                         air_temp, s_controller_state.target_temp, s_controller_state.config.full_power_delta);
                s_controller_state.state = CONTROLLER_STATE_HEATING_FULL_POWER;
                set_heat_power(255);
            }
            else
            {
                set_heat_power(0); // Ensure heater is off in IDLE
            }
            break;

        case CONTROLLER_STATE_HEATING_FULL_POWER:
            if (heater_temp >= s_controller_state.config.max_heater_temp)
            {
                ESP_LOGI(TAG, "HEATER Temp %.2fC >= Max Heater Temp %.2fC. Transitioning to MODULATING_HEATER_TEMP.",
                         heater_temp, s_controller_state.config.max_heater_temp);
                s_controller_state.state = CONTROLLER_STATE_MODULATING_HEATER_TEMP;
                set_heat_power(255); // Start modulating, might immediately turn off based on current temp
            }
            else if (air_temp >= (s_controller_state.target_temp - s_controller_state.config.air_temp_hysteresis))
            {
                // Air temp is approaching target, bypass modulating heater temp state
                ESP_LOGI(TAG, "AIR Temp %.2fC approaching Target %.2fC. Transitioning to MAINTAINING_AIR_TEMP.",
                         air_temp, s_controller_state.target_temp);
                s_controller_state.state = CONTROLLER_STATE_MAINTAINING_AIR_TEMP;
                // Power will be set by MAINTAINING_AIR_TEMP logic
            }
            else
            {
                set_heat_power(255); // Continue full power
            }
            break;

        case CONTROLLER_STATE_MODULATING_HEATER_TEMP:
            if (air_temp >= (s_controller_state.target_temp - s_controller_state.config.air_temp_hysteresis))
            {
                ESP_LOGI(TAG, "AIR Temp %.2fC approaching Target %.2fC. Transitioning to MAINTAINING_AIR_TEMP.",
                         air_temp, s_controller_state.target_temp);
                s_controller_state.state = CONTROLLER_STATE_MAINTAINING_AIR_TEMP;
                // Power will be set by MAINTAINING_AIR_TEMP logic
            }
            else if (heater_temp > s_controller_state.config.max_heater_temp)
            {
                set_heat_power(0); // Exceeded max heater temp, turn off
            }
            else if (heater_temp < (s_controller_state.config.max_heater_temp - s_controller_state.config.heater_temp_hysteresis))
            {
                set_heat_power(255); // Below lower bound, turn on
            }
            break;

        case CONTROLLER_STATE_MAINTAINING_AIR_TEMP:
            if (air_temp > (s_controller_state.target_temp + s_controller_state.config.air_temp_hysteresis))
            {
                set_heat_power(0); // Above target hysteresis, turn off
            }
            else if (air_temp < (s_controller_state.target_temp - s_controller_state.config.air_temp_hysteresis))
            {
                set_heat_power(255); // Below target hysteresis, turn on full power
            }
            // No else, power remains as is if within hysteresis band
            break;

        default:
            ESP_LOGE(TAG, "Unknown controller state: %d. Forcing IDLE.", s_controller_state.state);
            s_controller_state.state = CONTROLLER_STATE_IDLE;
            set_heat_power(0);
            break;
        }

        ESP_LOGD(TAG, "State: %d, Heater: %.2fC, Air: %.2fC, Target: %.2fC, Power: %u",
                 s_controller_state.state, heater_temp, air_temp, s_controller_state.target_temp, s_controller_state.current_power);

        xSemaphoreGive(s_controller_state.mutex);
    }
    else
    {
        ESP_LOGE(TAG, "Failed to take mutex to run controller.");
    }
}
