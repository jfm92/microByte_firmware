idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS ""
                    REQUIRES lvgl)

target_compile_definitions(${COMPONENT_LIB} PRIVATE LV_CONF_INCLUDE_SIMPLE=1)