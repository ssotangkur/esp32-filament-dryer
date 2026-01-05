idf_component_register(SRCS "ota.c" "sysmon_wrapper.c" "hello_world_main.c"
                                 "startup_tests.c"
                                 "display.c"
                                 "diagnostic.c"
                                 "wifi.c"
                                 "temp.c"
                                 "version.c"
                                 "circular_buffer.c"
                                 "web/web_server_init.c"
                                 "web/web_handlers.c"
                                 "web/web_websocket.c"

                       PRIV_REQUIRES spi_flash esp_lcd lvgl esp_lvgl_port driver
                                     esp_wifi nvs_flash esp_netif freertos
                                     ssotangkur__sysmon app_update esp_http_client esp_https_ota
                                     esp_adc joltwallet__littlefs esp_http_server
                       INCLUDE_DIRS "../include" "${CMAKE_BINARY_DIR}/include"
                       EXCLUDE_SRCS "managed_components/ssotangkur__sysmon/src/sysmon_http.c"
                                    "managed_components/ssotangkur__sysmon/src/sysmon_handlers.c"
                                    "managed_components/ssotangkur__sysmon/src/sysmon_json.c")

# Custom target to build web app before creating littleFS image
add_custom_target(build_web_app
    COMMAND powershell -Command "cd web; npm run build"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building web app"
)

littlefs_create_partition_image(littlefs ../web/dist FLASH_IN_PROJECT DEPENDS build_web_app)
