/**
 * Mock header file for ESP-IDF ADC functions used in unit tests
 * This file contains simplified declarations of ADC functions that will be mocked by CMock
 */

#pragma once

#include <stdint.h>
#include <stdbool.h>

// ESP-IDF ADC types (simplified for mocking)
typedef enum
{
  ADC_UNIT_1 = 0,
  ADC_UNIT_2 = 1
} adc_unit_t;

typedef enum
{
  ADC_CHANNEL_0 = 0,
  ADC_CHANNEL_1 = 1,
  ADC_CHANNEL_2 = 2,
  ADC_CHANNEL_3 = 3,
  ADC_CHANNEL_4 = 4,
  ADC_CHANNEL_5 = 5,
  ADC_CHANNEL_6 = 6,
  ADC_CHANNEL_7 = 7
} adc_channel_t;

typedef enum
{
  ADC_ATTEN_DB_0 = 0,
  ADC_ATTEN_DB_2_5 = 1,
  ADC_ATTEN_DB_6 = 2,
  ADC_ATTEN_DB_12 = 3
} adc_atten_t;

typedef enum
{
  ADC_BITWIDTH_9 = 0,
  ADC_BITWIDTH_10 = 1,
  ADC_BITWIDTH_11 = 2,
  ADC_BITWIDTH_12 = 3
} adc_bitwidth_t;

typedef enum
{
  ADC_ULP_MODE_DISABLE = 0,
  ADC_ULP_MODE_ENABLE = 1
} adc_ulp_mode_t;

// ADC handle types
typedef void *adc_oneshot_unit_handle_t;
typedef void *adc_cali_handle_t;

// ADC configuration structures (simplified)
typedef struct
{
  adc_unit_t unit_id;
  adc_ulp_mode_t ulp_mode;
} adc_oneshot_unit_init_cfg_t;

typedef struct
{
  adc_atten_t atten;
  adc_bitwidth_t bitwidth;
} adc_oneshot_chan_cfg_t;

typedef struct
{
  adc_unit_t unit_id;
  adc_atten_t atten;
  adc_bitwidth_t bitwidth;
} adc_cali_curve_fitting_config_t;

// ESP-IDF error codes
typedef enum
{
  ESP_OK = 0,
  ESP_FAIL = -1,
  ESP_ERR_INVALID_ARG = 0x102,
  ESP_ERR_INVALID_STATE = 0x103,
} esp_err_t;

// ADC functions that need to be mocked
esp_err_t adc_oneshot_new_unit(const adc_oneshot_unit_init_cfg_t *init_config, adc_oneshot_unit_handle_t *ret_unit);
esp_err_t adc_oneshot_config_channel(adc_oneshot_unit_handle_t handle, adc_channel_t channel, const adc_oneshot_chan_cfg_t *config);
esp_err_t adc_oneshot_read(adc_oneshot_unit_handle_t handle, adc_channel_t channel, int *out_raw);
void adc_oneshot_del_unit(adc_oneshot_unit_handle_t handle);

esp_err_t adc_cali_create_scheme_curve_fitting(const adc_cali_curve_fitting_config_t *config, adc_cali_handle_t *ret_handle);
esp_err_t adc_cali_raw_to_voltage(adc_cali_handle_t handle, int adc_raw, int *voltage_mv);
void adc_cali_delete_scheme_curve_fitting(adc_cali_handle_t handle);

// Utility functions
const char *esp_err_to_name(esp_err_t code);
