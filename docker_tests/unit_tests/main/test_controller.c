#include "unity.h"
#include "heater_controller.h"

// Mocks - Include the generated mock headers
#include "Mockmock_heater.h"
#include "Mockmock_semphr.h"

// Do NOT include controller.c directly anymore, it will be linked
// #include "controller.c"

static const controller_config_t TEST_CONFIG = {
    .max_heater_temp = 100.0f,
    .air_temp_hysteresis = 1.0f,
    .heater_temp_hysteresis = 2.0f,
    .full_power_delta = 5.0f,
};

// Dummy mutex handle
static SemaphoreHandle_t s_test_mutex = (SemaphoreHandle_t)0x1234;

// Helper function to initialize controller for tests
void setup_controller_test(const controller_config_t *config, float initial_target_temp)
{
    xSemaphoreCreateMutex_ExpectAndReturn(s_test_mutex);
    controller_init(config, initial_target_temp);
}

// Helper function to deinitialize controller for tests
void teardown_controller_test(void)
{
    vSemaphoreDelete_Expect(s_test_mutex);
    controller_deinit();
}

void test_controller_init_success(void)
{
    // Setup
    setup_controller_test(&TEST_CONFIG, 50.0f);
    // Assertions
    TEST_ASSERT_TRUE(controller_get_state()->initialized);
    TEST_ASSERT_EQUAL_FLOAT(TEST_CONFIG.max_heater_temp, controller_get_state()->config.max_heater_temp);
    TEST_ASSERT_EQUAL_FLOAT(50.0f, controller_get_state()->target_temp);
    TEST_ASSERT_TRUE(controller_get_state()->active);
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, controller_get_state()->state);
    TEST_ASSERT_EQUAL_PTR(s_test_mutex, controller_get_state()->mutex);
    // Teardown
    teardown_controller_test();
}

void test_controller_init_failure_null_config(void)
{
    // Call function under test with NULL config
    controller_init(NULL, 50.0f);

    // Assertions
    TEST_ASSERT_FALSE(controller_get_state()->initialized);
    TEST_ASSERT_NULL(controller_get_state()->mutex);
}

void test_controller_init_failure_mutex_create(void)
{
    xSemaphoreCreateMutex_ExpectAndReturn(NULL); // Mock mutex creation failures

    // Call function under test
    controller_init(&TEST_CONFIG, 50.0f);

    // Assertions
    TEST_ASSERT_FALSE(controller_get_state()->initialized);
    TEST_ASSERT_NULL(controller_get_state()->mutex); // Should be NULL if creation failed
}

void test_controller_deinit_not_initialized(void)
{
    // Setup - Ensure state is not initialized and initialize esp_log mock for the warning log
    controller_internal_state_t *state = controller_get_state();
    state->initialized = false;

    // Call deinit on uninitialized controller
    controller_deinit(); // Should just log a warning and do nothing
    TEST_ASSERT_FALSE(state->initialized);
    TEST_ASSERT_NULL(state->mutex);
}

void test_controller_set_target_temp(void)
{
    // Setup - initialize controller
    setup_controller_test(&TEST_CONFIG, 20.0f); // Initial target temp
    // Set new target temp
    float new_target_temp = 60.0f;
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE); // Expect mutex take
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);                // Expect mutex give
    controller_set_target_temp(new_target_temp);
    // Assertions
    TEST_ASSERT_EQUAL_FLOAT(new_target_temp, controller_get_state()->target_temp);
    // Teardown
    teardown_controller_test();
}

void test_controller_set_active(void)
{
    // Setup - initialize controller
    setup_controller_test(&TEST_CONFIG, 20.0f); // Initial target temp
    controller_internal_state_t *state = controller_get_state();

    // Set active to false
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    controller_set_active(false);
    // Assertions
    TEST_ASSERT_FALSE(state->active);

    // Set active to true
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    controller_set_active(true);
    TEST_ASSERT_TRUE(state->active);

    // Teardown
    teardown_controller_test();
}

// Tests for controller_run()
void test_controller_run_uninitialized(void)
{
    controller_internal_state_t *state = controller_get_state();
    state->initialized = false;   // Ensure not initialized
    controller_run(25.0f, 20.0f); // Should do nothing and log a warning
    TEST_ASSERT_FALSE(state->initialized);
}

void test_controller_run_mutex_failure(void)
{
    xSemaphoreCreateMutex_ExpectAndReturn(s_test_mutex);
    controller_init(&TEST_CONFIG, 50.0f);

    // Expect mutex take to fail
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdFALSE);

    controller_run(25.0f, 20.0f); // Should log an error and return

    // Assertions (state should not have changed)
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, controller_get_state()->state);

    // Teardown
    teardown_controller_test();
}

void test_controller_run_idle_no_transition(void)
{
    // Setup
    setup_controller_test(&TEST_CONFIG, 50.0f); // Initial target 50
    controller_internal_state_t *state = controller_get_state();

    // Expect mutex take/give for controller_run
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    set_heat_power_Expect(0);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);

    // Run controller with temps that keep it in IDLE (air temp >= target - full_power_delta)
    controller_run(25.0f, 46.0f); // Target 50, delta 5.0 -> 50 - 5 = 45. 46 > 45.

    // Assertions
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state);

    // Teardown
    teardown_controller_test();
}

void test_controller_run_idle_transition_to_full_power(void)
{
    // Setup
    setup_controller_test(&TEST_CONFIG, 50.0f); // Initial target 50
    controller_internal_state_t *state = controller_get_state();

    // Expect mutex take/give for controller_run
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    // Expect set_heat_power to 0 initially, then to 255 for transition
    set_heat_power_Expect(255); // For HEATING_FULL_POWER
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);

    // Run controller with temps that trigger transition (air temp < target - full_power_delta)
    controller_run(25.0f, 44.0f); // Target 50, delta 5.0 -> 50 - 5 = 45. 44 < 45.

    // Assertions
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state);

    // Teardown
    teardown_controller_test();
}

void test_controller_global_safety_override(void)
{
    // Setup - controller initialized to a state where it would normally be heating
    float initial_target = 50.0f;
    setup_controller_test(&TEST_CONFIG, initial_target);
    controller_internal_state_t *state = controller_get_state();

    // Simulate transition to HEATING_FULL_POWER state
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    set_heat_power_Expect(255); // For HEATING_FULL_POWER
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);

    controller_run(30.0f, 40.0f); // heater_temp < max, air_temp < target - delta
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state);

    // --- Scenario 1: Heater temp exceeds max_heater_temp ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    set_heat_power_Expect(0); // Expect power off
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);

    controller_run(TEST_CONFIG.max_heater_temp + 1.0f, 60.0f); // Heater temp exceeds max
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state);    // Should switch to IDLE
    TEST_ASSERT_TRUE(state->heater_safety_override_active);    // Safety override should be active

    // --- Scenario 2: Heater remains hot, tries to heat but should stay IDLE ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);                                             // For controller_run log
    controller_run(TEST_CONFIG.max_heater_temp - (TEST_CONFIG.heater_temp_hysteresis / 2.0f), 40.0f); // Still above threshold for resuming
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state);                                           // Should remain IDLE
    TEST_ASSERT_TRUE(state->heater_safety_override_active);                                           // Safety override should still be active

    // --- Scenario 3: Heater cools down below threshold, normal control resumes ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    set_heat_power_Expect(255);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    // Expect full power to resume                                                         // For controller_run log
    controller_run(TEST_CONFIG.max_heater_temp - TEST_CONFIG.heater_temp_hysteresis - 1.0f, 40.0f); // Heater temp below threshold
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state);                           // Should resume heating
    TEST_ASSERT_FALSE(state->heater_safety_override_active);                                        // Safety override should be inactive

    // Teardown
    teardown_controller_test();
}

// Test group runner for controller tests
void test_controller_group_runner(void)
{
    printf("Running controller tests...\n");
    RUN_TEST(test_controller_init_success);
    RUN_TEST(test_controller_init_failure_null_config);
    RUN_TEST(test_controller_init_failure_mutex_create);
    RUN_TEST(test_controller_deinit_not_initialized);
    RUN_TEST(test_controller_set_target_temp); // New test
    RUN_TEST(test_controller_set_active);
    RUN_TEST(test_controller_run_uninitialized);
    RUN_TEST(test_controller_run_mutex_failure);
    RUN_TEST(test_controller_run_idle_no_transition);
    RUN_TEST(test_controller_run_idle_transition_to_full_power);
    RUN_TEST(test_controller_global_safety_override); // Add new test
    printf("Controller tests completed\n");
}
