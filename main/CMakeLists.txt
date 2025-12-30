idf_component_register(SRCS "ota.c" "sysmon_wrapper.c" "hello_world_main.c"
                                 "startup_tests.c"
                                 "display.c"
                                 "diagnostic.c"
                                 "wifi.c"
                                 "temp.c"
                                 "version.c"
                                 "circular_buffer.c"

                       PRIV_REQUIRES spi_flash esp_lcd lvgl esp_lvgl_port driver
                                     esp_wifi nvs_flash esp_netif freertos
                                     ssotangkur__sysmon app_update esp_http_client esp_https_ota
                                     esp_adc
                       INCLUDE_DIRS "../include" "${CMAKE_BINARY_DIR}/include")
