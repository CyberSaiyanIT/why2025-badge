cmake_minimum_required(VERSION 3.16.0)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(badge-firmware)


# For LVGL
if(CONFIG_FREERTOS_SMP)
    include_directories(${IDF_PATH}/components/freertos/FreeRTOS-Kernel-SMP/include/freertos/)
else()
    include_directories(${IDF_PATH}/components/freertos/FreeRTOS-Kernel/include/freertos/)
endif()
