idf_component_register(SRCS "espnow_example_main.c"
                    INCLUDE_DIRS "."
                    REQUIRES nvs_flash bt  esp_hid  esp_wifi ) 

target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")