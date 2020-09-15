#COMPONENT_ADD_INCLUDEDIRS := .
#COMPONENT_SRCDIRS := .

ifndef LV_CONF_INCLUDE_SIMPLE
CFLAGS += -DLV_CONF_INCLUDE_SIMPLE
endif

COMPONENT_SRCDIRS := LVGL/ \
	LVGL/src/lv_core \
	LVGL/src/lv_draw \
	LVGL/src/lv_widgets \
	LVGL/src/lv_hal \
	LVGL/src/lv_misc \
	LVGL/src/lv_themes \
	LVGL/src/lv_font \
	.

COMPONENT_ADD_INCLUDEDIRS := $(COMPONENT_SRCDIRS) .