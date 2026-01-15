# Implementation Plan: Heater Control System

This plan outlines the steps to implement the Heater Control System as defined in the `spec.md`.

## Phase 1: Heater Driver (Hardware Abstraction Layer) [checkpoint: ]

- [x] **Task:** Implement the `heater.h` header file. 8409220
    - [ ] Define the `heater_init()` and `set_heat_power()` function prototypes.
- [ ] **Task:** Implement the `heater.c` source file.
    - [ ] Write tests for the `heater` module (`docker_tests/unit_tests/main/test_heater.c`).
    - [ ] Implement `heater_init()` to configure the GPIO/PWM for the heater.
    - [ ] Implement `set_heat_power()` to control the heater's power level.
- [ ] **Task:** Conductor - User Manual Verification 'Heater Driver (Hardware Abstraction Layer)' (Protocol in workflow.md)

## Phase 2: Controller (Control Logic Layer) [checkpoint: ]

- [ ] **Task:** Implement the `controller.h` header file.
    - [ ] Define the `controller_config_t` struct.
    - [ ] Define the prototypes for `controller_init()`, `controller_deinit()`, `controller_set_target_temp()`, `controller_set_active()`, and `controller_run()`.
- [ ] **Task:** Implement the `controller.c` source file.
    - [ ] Write tests for the `controller` module (`docker_tests/unit_tests/main/test_controller.c`).
        - [ ] Mock `heater.h` interface.
        - [ ] Mock FreeRTOS mutex functions.
        - [ ] Test mutex initialization and deinitialization.
        - [ ] Test state transitions.
        - [ ] Test `set_heat_power` calls.
        - [ ] Test thread-safe updates of `target_temp`.
        - [ ] Test `controller_set_active`.
        - [ ] Test Global Safety Override.
    - [ ] Implement `controller_init()` to initialize the mutex and configuration.
    - [ ] Implement `controller_deinit()` to delete the mutex.
    - [ ] Implement `controller_set_target_temp()` with mutex protection.
    - [ ] Implement `controller_set_active()` with mutex protection.
    - [ ] Implement the `controller_run()` state machine logic with mutex protection.
- [ ] **Task:** Conductor - User Manual Verification 'Controller (Control Logic Layer)' (Protocol in workflow.md)

## Phase 3: Integration and Application Task [checkpoint: ]

- [ ] **Task:** Integrate the controller into the main application.
    - [ ] Create a dedicated FreeRTOS task to periodically read sensors and call `controller_run()`.
    - [ ] Initialize the controller with appropriate configuration values.
- [ ] **Task:** Conductor - User Manual Verification 'Integration and Application Task' (Protocol in workflow.md)
