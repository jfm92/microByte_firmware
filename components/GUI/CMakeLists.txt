file(GLOB_RECURSE SOURCES LVGL/src/*.c)
idf_component_register(SRCS ${SOURCES}
                       INCLUDE_DIRS . LVGL)

target_compile_definitions(${COMPONENT_LIB} INTERFACE LV_CONF_INCLUDE_SIMPLE=1)
