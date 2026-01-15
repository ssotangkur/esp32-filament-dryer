# Specification: Implement the Heater Control System

This track covers the implementation of the core temperature control system for the ESP32-based filament dryer.

## 1. Core Components

-   **Heater Driver:** A hardware abstraction layer (`heater.h`, `heater.c`) for direct, low-level control of the heater element's power level.
-   **Controller:** A high-level state machine (`controller.h`, `controller.c`) that implements the temperature control logic, makes decisions based on sensor inputs, and ensures thread-safe operation.

## 2. Key Features

-   **State-Machine Based Control:** A multi-stage control loop (IDLE, HEATING_FULL_POWER, MODULATING_HEATER_TEMP, MAINTAINING_AIR_TEMP) to intelligently manage heating.
-   **Heater Safety Limiter:** A global safety override to prevent the heater element from overheating.
-   **Runtime Configuration:** A thread-safe API to set the target air temperature and activate/deactivate the controller at runtime.

## 3. Execution Model

-   A periodic, polling-based model where a dedicated task reads sensors and calls the `controller_run()` function.
-   The controller's internal state will be protected by a FreeRTOS mutex.

## 4. Testing Strategy

-   Unit tests will be created for both the Heater Driver and the Controller.
-   The Controller tests will mock the Heater Driver and FreeRTOS mutex functions.
-   Testing will cover state transitions, power level outputs, thread safety, and safety overrides.
