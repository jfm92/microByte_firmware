#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := microByte


EXTRA_COMPONENT_DIRS := components/GUI \
						components/drivers/system_configuration \
						components/drivers/display/backlight_ctrl \
						components/drivers/display/ST7789 \
						components/drivers/display/display_HAL \
						components/drivers/user_input/TCA9555 \
						components/drivers/user_input/user_input_HAL \
						components/drivers/sd_storage \
						components/drivers/battery \
						components/drivers/sound \
						components/OTA \
						components/emulators/GBC \
						components/emulators/GBC/gnuboy \
						components/emulators/SMS \
						components/emulators/SMS/smsplus \
						components/emulators/NES \
						components/emulators/NES/nofrendo \
						components/boot_screen \
						components/boot_screen/font_render \
						components/drivers/LED \
						#components/emulators/SNES/snes9x \
						#components/emulators/SNES \


include $(IDF_PATH)/make/project.mk