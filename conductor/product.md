# Heater Control System - Product Requirements Document (PRD)

## 1. Overview

This document specifies the requirements for a temperature control system for an ESP32-based filament dryer. The system will manage a heater element to achieve and maintain a target air temperature, while ensuring the heater itself does not exceed a specified maximum safe temperature. The system must be robust, testable, and allow for runtime adjustments of the target temperature via a thread-safe API.

**Primary Objectives:**
*   Maintain optimal filament humidity to prevent print failures.
*   Provide remote monitoring and control capabilities for convenience.

## 2. Core Features

-   **State-Machine Based Temperature Control:** A multi-stage control loop to intelligently manage heating.
-   **Heater Safety Limiter:** A hard safety override to prevent the heater element from overheating.
-   **Runtime Configuration:** The target air temperature can be updated at any time from any task (e.g., a REST API handler).
-   **Modular, Decoupled Architecture:** A clear separation between the low-level heater driver and the high-level control logic.

## 3. Component Architecture

The feature will be implemented across two new, distinct components.

### 3.1. Heater Driver (Hardware Abstraction Layer)

This component is responsible for direct, low-level control of the heater hardware.

-   **Files:**
    -   Header: `include/heater.h`
    -   Source: `main/heater.c`
    -   Test: `docker_tests/unit_tests/main/test_heater.c`
-   **Responsibilities:**
    -   Initialize the GPIO and/or PWM peripherals used to power the heater.
    -   Provide a simple function to set the heater's power level.
-   **API:**
    ```c
    /** @brief Initializes the heater hardware peripherals. */
    void heater_init(void);

    /**
     * @brief Sets the heater power level.
     * @param power Power level from 0 (off) to 255 (max).
     */
    void set_heat_power(uint8_t power);
    ```

### 3.2. Controller (Control Logic Layer)

This component contains the high-level state machine and logic to manage the temperature control loop.

-   **Files:**
    -   Header: `include/controller.h`
    -   Source: `main/controller.c`
    -   Test: `docker_tests/unit_tests/main/test_controller.c`
-   **Responsibilities:**
    -   Implement the control state machine.
    -   Make decisions based on temperature inputs.
    -   Call the Heater Driver API (`set_heat_power`) to enact decisions.
    -   Ensure thread-safe access to its internal state.
-   **API:**
    ```c
    /** @brief Configuration parameters for the heater controller. */
    typedef struct {
        float max_heater_temp;          // Max safe temperature for the heater element.
        float air_temp_hysteresis;      // Hysteresis for air temp bang-bang control (e.g., 1.0f).
        float heater_temp_hysteresis;   // Hysteresis for heater temp bang-bang control (e.g., 2.0f).
        float full_power_delta;         // Temp delta below target to engage full power mode (e.g. 5.0f).
    } controller_config_t;

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
    ```

## 4. Execution Model and Thread Safety

-   **Execution Model:** A periodic, polling-based model will be used. A dedicated FreeRTOS task (or an existing application task) will be responsible for:
    1.  Periodically reading the heater and air temperature sensors.
    2.  Passing the new temperature values to the `controller_run()` function.
    3.  Delaying for a fixed interval (e.g., 1 second) before repeating the cycle.
-   **Data Flow:** The controller is a passive component. It receives temperature data via the `controller_run()` parameters; it does not fetch data itself.
-   **Thread Safety:** The controller's internal state (e.g., target temperature, current state) must be protected from concurrent access. A FreeRTOS mutex will be created in `controller_init()` and used to protect all read/write access to the internal state within all public API functions (`controller_set_target_temp`, `controller_run`).

## 5. Control Logic (State Machine)

The controller will operate as a state machine. A global safety override will supersede all state logic.

#### **Global Control State**

-   **Condition:** If the controller is deactivated via `controller_set_active(false)`.
-   **Action:** Immediately call `set_heat_power(0)` and force the controller into the `IDLE` state, regardless of any other conditions or target temperatures. Control logic will be bypassed.

#### **Global Safety Override**

-   **Condition:** If at any point `heater_temp` exceeds `config.max_heater_temp`.
-   **Action:** Immediately call `set_heat_power(0)`. The controller must not command any power output until `heater_temp` falls below `config.max_heater_temp - config.heater_temp_hysteresis`.

#### **States**

1.  **IDLE**
    -   **Action:** `set_heat_power(0)`.
    -   **Entry Condition:** The controller is deactivated via `controller_set_active(false)` or the `target_temp` is set at or below the ambient air temperature.

2.  **HEATING_FULL_POWER** (Ramping)
    -   **Entry Condition:** `air_temp < (target_temp - config.full_power_delta)`.
    -   **Action:** `set_heat_power(255)`.
    -   **Transition to `MODULATING_HEATER_TEMP`:** When `heater_temp` approaches `config.max_heater_temp`.

3.  **MODULATING_HEATER_TEMP** (Heater Temp Limiting)
    -   **Entry Condition:** `heater_temp` is at its limit, but `air_temp` is still below target.
    -   **Action:** Use bang-bang control to keep the heater at its max temp.
        -   If `heater_temp >= config.max_heater_temp`, `set_heat_power(0)`.
        -   If `heater_temp < config.max_heater_temp - config.heater_temp_hysteresis`, `set_heat_power(255)`.
    -   **Transition to `MAINTAINING_AIR_TEMP`:** When `air_temp` enters the `target_temp +/- config.air_temp_hysteresis` band.

4.  **MAINTAINING_AIR_TEMP** (Target Temp Maintenance)
    -   **Entry Condition:** `air_temp` is within the target hysteresis band.
    -   **Action:** Use bang-bang control to maintain `air_temp`. This is always subject to the Global Safety Override.
                - If `air_temp > target_temp + config.air_temp_hysteresis`, `set_heat_power(0)`.
                - If `air_temp < target_temp - config.air_temp_hysteresis`, `set_heat_power(255)`.

## 6. Testing Strategy

-   **Unit Tests:** Two new test suites will be created: `test_heater.c` and `test_controller.c`.
-   **Mocks:** The `test_controller` suite will require mocking:
    -   The `heater.h` interface (`set_heat_power`).
    -   FreeRTOS mutex functions (`xSemaphoreCreateMutex`, `xSemaphoreTake`, `xSemaphoreGive`, `vSemaphoreDelete`), using the existing `mock_semphr.h` framework.
-   **Key `test_controller` Scenarios:**
    -   Verify correct initialization and deinitialization of the mutex.
    -   Verify correct state transitions based on a series of simulated temperature inputs.
    -   Verify correct `set_heat_power` calls and power levels for each state.
    -   Verify thread-safe update of `target_temp` via `controller_set_target_temp`.
    -   Verify the functionality of `controller_set_active`: when deactivated, `set_heat_power(0)` is called, and the controller remains idle. When reactivated, normal control resumes.
    -   Verify the Global Safety Override: simulate `heater_temp` exceeding the max limit and assert that `set_heat_power(0)` is called immediately and that heating does not resume prematurely.
