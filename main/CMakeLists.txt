file(GLOB_RECURSE SRCS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "*.c")
list(FILTER SRCS EXCLUDE REGEX "managed_components/.*")
idf_component_register(SRCS ${SRCS}
    PRIV_REQUIRES spi_flash esp_lcd lvgl esp_lvgl_port driver
        esp_wifi nvs_flash esp_netif freertos
        ssotangkur__sysmon app_update esp_http_client esp_https_ota
        esp_adc joltwallet__littlefs esp_http_server
    INCLUDE_DIRS "../include" "${CMAKE_BINARY_DIR}/include"
    EXCLUDE_SRCS "managed_components/**/*.c" "web/*.c")

# Custom target to build web app before creating littleFS image
add_custom_target(build_web_app
    COMMAND powershell -Command "cd web; npm run build"
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Building web app"
)

littlefs_create_partition_image(littlefs ../web/dist FLASH_IN_PROJECT DEPENDS build_web_app)
