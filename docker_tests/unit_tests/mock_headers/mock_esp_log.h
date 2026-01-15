#ifndef MOCK_ESP_LOG_H
#define MOCK_ESP_LOG_H

#include "cmock.h"
#include "esp_log_wrapper.h" // Include the wrapper header

// Mock declarations for esp_log_wrapper_i
void esp_log_wrapper_i_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void esp_log_wrapper_i_CMockExpect(UNITY_LINE_TYPE cmock_line, const char *tag, const char *format);

// Mock declarations for esp_log_wrapper_d
void esp_log_wrapper_d_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void esp_log_wrapper_d_CMockExpect(UNITY_LINE_TYPE cmock_line, const char *tag, const char *format);

// Mock declarations for esp_log_wrapper_w
void esp_log_wrapper_w_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void esp_log_wrapper_w_CMockExpect(UNITY_LINE_TYPE cmock_line, const char *tag, const char *format);

// Mock declarations for esp_log_wrapper_e
void esp_log_wrapper_e_CMockExpectAnyArgs(UNITY_LINE_TYPE cmock_line);
void esp_log_wrapper_e_CMockExpect(UNITY_LINE_TYPE cmock_line, const char *tag, const char *format);

#endif // MOCK_ESP_LOG_H
