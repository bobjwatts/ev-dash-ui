cmake_minimum_required(VERSION 3.16)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)

set(EXTRA_COMPONENT_DIRS "${CMAKE_SOURCE_DIR}/components")

add_compile_options(-Wno-format)
add_compile_options(-Wno-ignored-qualifiers)
add_compile_definitions(LV_LVGL_H_INCLUDE_SIMPLE)

project(ev_dash_p4)
