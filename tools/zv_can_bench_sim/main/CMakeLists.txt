idf_component_register(SRCS "main.c" REQUIRES esp_driver_twai esp_timer freertos)

target_include_directories(${COMPONENT_LIB} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/../../../firmware/components/zv_can")
