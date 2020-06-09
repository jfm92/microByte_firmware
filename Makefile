#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := micro-gnuboy

EXTRA_COMPONENT_DIRS := components/lv_port_esp32/components/lvgl \
						components/drivers/spi \
						components/drivers/system_configuration \
						components/drivers/st7789 \
						components/drivers/display_hal \
						components/drivers/user_input \
						components/drivers/external_flash \
						components/gnuboy_core \


include $(IDF_PATH)/make/project.mk