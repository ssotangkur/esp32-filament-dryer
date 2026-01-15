#include "unity.h"
#include "controller.h"

// Mocks - Include the generated mock headers
#include "Mockmock_heater.h"
#include "Mockmock_semphr.h"
#include "mock_esp_log.h" // Include the mock for esp_log

// Do NOT include controller.c directly anymore, it will be linked
// #include "controller.c"

// Access to internal state for testing purposes only
typedef enum {
    CONTROLLER_STATE_IDLE,
    CONTROLLER_STATE_HEATING_FULL_POWER,
    CONTROLLER_STATE_MODULATING_HEATER_TEMP,
    CONTROLLER_STATE_MAINTAINING_AIR_TEMP,
} controller_state_t;

typedef struct {
    controller_config_t config;
    float target_temp;
    bool active;
    controller_state_t state;
    SemaphoreHandle_t mutex;
    bool initialized; // Flag to indicate if controller is initialized
    bool heater_safety_override_active; // Flag to indicate if safety override is active
    uint8_t current_power; // Current power level commanded to the heater
} controller_internal_state_t;



static const controller_config_t TEST_CONFIG = {
    .max_heater_temp = 100.0f,
    .air_temp_hysteresis = 1.0f,
    .heater_temp_hysteresis = 2.0f,
    .full_power_delta = 5.0f,
};

// Dummy mutex handle
static SemaphoreHandle_t s_test_mutex = (SemaphoreHandle_t)0x1234;

// Helper function to initialize controller for tests
void setup_controller_test(const controller_config_t *config, float initial_target_temp) {
    Mockmock_semphr_Init();
    Mockmock_heater_Init();
    Mockmock_esp_log_Init(); // Initialize esp_log mock
    xSemaphoreCreateMutex_ExpectAndReturn(s_test_mutex);
    controller_init(config, initial_target_temp);
    // Expect heater to be set to 0 power on initialization
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 0);
    // Expect controller init log
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM);
}

// Helper function to deinitialize controller for tests
void teardown_controller_test(void) {
    vSemaphoreDelete_Expect(s_test_mutex);
    controller_deinit();
    // Expect controller deinit log
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM);
    Mockmock_semphr_Verify();
    Mockmock_semphr_Destroy();
    Mockmock_heater_Verify();
    Mockmock_heater_Destroy();
    Mockmock_esp_log_Verify(); // Verify esp_log mock
    Mockmock_esp_log_Destroy(); // Destroy esp_log mock
}


void test_controller_init_success(void) {
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

void test_controller_init_failure_null_config(void) {
    // Setup - no mutex creation expected
    Mockmock_semphr_Init();
    Mockmock_esp_log_Init(); // Initialize esp_log mock for the error log
    esp_log_wrapper_e_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect error log for NULL config
    
    // Call function under test with NULL config
    controller_init(NULL, 50.0f);

    // Assertions
    TEST_ASSERT_FALSE(controller_get_state()->initialized);
    TEST_ASSERT_NULL(controller_get_state()->mutex);

    Mockmock_semphr_Verify();
    Mockmock_semphr_Destroy();
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_controller_init_failure_mutex_create(void) {
    // Setup - Expect malloc to succeed, mutex create to fail
    Mockmock_semphr_Init();
    Mockmock_heater_Init(); // heater_init will be called internally from controller_init
    Mockmock_esp_log_Init(); // Initialize esp_log mock for the error log
    xSemaphoreCreateMutex_ExpectAndReturn(NULL); // Mock mutex creation failure
    esp_log_wrapper_e_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect error log for mutex creation failure
    
    // Call function under test
    controller_init(&TEST_CONFIG, 50.0f);

    // Assertions
    TEST_ASSERT_FALSE(controller_get_state()->initialized);
    TEST_ASSERT_NULL(controller_get_state()->mutex); // Should be NULL if creation failed

    Mockmock_semphr_Verify();
    Mockmock_semphr_Destroy();
    Mockmock_heater_Verify();
    Mockmock_heater_Destroy(); // Clean up mock after use
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_controller_deinit_not_initialized(void) {
    // Setup - Ensure state is not initialized and initialize esp_log mock for the warning log
    controller_internal_state_t* state = controller_get_state();
    state->initialized = false;
    state->mutex = NULL;
    Mockmock_esp_log_Init();
    esp_log_wrapper_w_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect warning log

    // Call deinit on uninitialized controller
    controller_deinit(); // Should just log a warning and do nothing
    TEST_ASSERT_FALSE(state->initialized);
    TEST_ASSERT_NULL(state->mutex);
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_controller_set_target_temp(void) {
    // Setup - initialize controller
    setup_controller_test(&TEST_CONFIG, 20.0f); // Initial target temp
    // Set new target temp
    float new_target_temp = 60.0f;
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE); // Expect mutex take
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE); // Expect mutex give
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect debug log for setting target temp
    controller_set_target_temp(new_target_temp);
    // Assertions
    TEST_ASSERT_EQUAL_FLOAT(new_target_temp, controller_get_state()->target_temp);
    // Teardown
    teardown_controller_test();
}


void test_controller_set_active(void) {
    // Setup - initialize controller
    setup_controller_test(&TEST_CONFIG, 20.0f); // Initial target temp
    controller_internal_state_t* state = controller_get_state();

    // Set active to false
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect info log
    controller_set_active(false);
    // Assertions
    TEST_ASSERT_FALSE(state->active);

    // Set active to true
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect info log
    controller_set_active(true);
    TEST_ASSERT_TRUE(state->active);

    // Teardown
    teardown_controller_test();
}


// Tests for controller_run()
void test_controller_run_uninitialized(void) {
    controller_internal_state_t* state = controller_get_state();
    state->initialized = false; // Ensure not initialized
    Mockmock_esp_log_Init();
    esp_log_wrapper_w_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect warning log
    controller_run(25.0f, 20.0f); // Should do nothing and log a warning
    TEST_ASSERT_FALSE(state->initialized);
    Mockmock_esp_log_Verify();
    Mockmock_esp_log_Destroy();
}

void test_controller_run_mutex_failure(void) {
    // Setup - partial init to allow mutex creation mock
    Mockmock_semphr_Init();
    Mockmock_heater_Init();
    Mockmock_esp_log_Init();
    xSemaphoreCreateMutex_ExpectAndReturn(s_test_mutex);
    controller_init(&TEST_CONFIG, 50.0f);
    // Expect heater to be set to 0 power on initialization
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 0);
    // Expect controller init log
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM);
    
    // Expect mutex take to fail
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdFALSE);
    esp_log_wrapper_e_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect error log
    
    controller_run(25.0f, 20.0f); // Should log an error and return

    // Assertions (state should not have changed)
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, controller_get_state()->state);

    // Teardown
    teardown_controller_test();
}

void test_controller_run_idle_no_transition(void) {
    // Setup
    setup_controller_test(&TEST_CONFIG, 50.0f); // Initial target 50
    controller_internal_state_t* state = controller_get_state();

    // Expect mutex take/give for controller_run
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    // Expect set_heat_power to 0 as it's idle
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 0);
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect debug log

    // Run controller with temps that keep it in IDLE (air temp >= target - full_power_delta)
    controller_run(25.0f, 46.0f); // Target 50, delta 5.0 -> 50 - 5 = 45. 46 > 45.
    
    // Assertions
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state);
    
    // Teardown
    teardown_controller_test();
}

void test_controller_run_idle_transition_to_full_power(void) {
    // Setup
    setup_controller_test(&TEST_CONFIG, 50.0f); // Initial target 50
    controller_internal_state_t* state = controller_get_state();

    // Expect mutex take/give for controller_run
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    // Expect set_heat_power to 0 initially, then to 255 for transition
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 255); // For HEATING_FULL_POWER
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect info log for state transition
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect debug log

    // Run controller with temps that trigger transition (air temp < target - full_power_delta)
    controller_run(25.0f, 44.0f); // Target 50, delta 5.0 -> 50 - 5 = 45. 44 < 45.
    
    // Assertions
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state);
    
    // Teardown
    teardown_controller_test();
}

void test_controller_global_safety_override(void) {
    // Setup - controller initialized to a state where it would normally be heating
    float initial_target = 50.0f;
    setup_controller_test(&TEST_CONFIG, initial_target);
    controller_internal_state_t* state = controller_get_state();

    // Simulate transition to HEATING_FULL_POWER state
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 255);
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // For state transition
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // For controller_run log
    controller_run(30.0f, 40.0f); // heater_temp < max, air_temp < target - delta
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state);

    // --- Scenario 1: Heater temp exceeds max_heater_temp ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 0); // Expect power off
    esp_log_wrapper_w_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect warning log
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // For controller_run log
    controller_run(TEST_CONFIG.max_heater_temp + 1.0f, 60.0f); // Heater temp exceeds max
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state); // Should switch to IDLE
    TEST_ASSERT_TRUE(state->heater_safety_override_active); // Safety override should be active

    // --- Scenario 2: Heater remains hot, tries to heat but should stay IDLE ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect debug log for remaining IDLE
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // For controller_run log
    controller_run(TEST_CONFIG.max_heater_temp - (TEST_CONFIG.heater_temp_hysteresis / 2.0f), 40.0f); // Still above threshold for resuming
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_IDLE, state->state); // Should remain IDLE
    TEST_ASSERT_TRUE(state->heater_safety_override_active); // Safety override should still be active

    // --- Scenario 3: Heater cools down below threshold, normal control resumes ---
    xSemaphoreTake_ExpectAndReturn(s_test_mutex, portMAX_DELAY, pdTRUE);
    xSemaphoreGive_ExpectAndReturn(s_test_mutex, pdTRUE);
    set_heat_power_CMockExpect(UNITY_LINE_NUM, 255); // Expect full power to resume
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect info log for exiting safety override
    esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_NUM); // Expect info log for state transition
    esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_NUM); // For controller_run log
    controller_run(TEST_CONFIG.max_heater_temp - TEST_CONFIG.heater_temp_hysteresis - 1.0f, 40.0f); // Heater temp below threshold
    TEST_ASSERT_EQUAL(CONTROLLER_STATE_HEATING_FULL_POWER, state->state); // Should resume heating
    TEST_ASSERT_FALSE(state->heater_safety_override_active); // Safety override should be inactive

    // Teardown
    teardown_controller_test();
}

// Test group runner for controller tests
void test_controller_group_runner(void) {
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
