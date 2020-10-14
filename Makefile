#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

PROJECT_NAME := micro-gnuboy


EXTRA_COMPONENT_DIRS := components/drivers/spi \
						components/drivers/system_configuration \
						components/drivers/st7789 \
						components/drivers/display_hal \
						components/drivers/user_input \
						components/gnuboy_core \
						components/drivers/sound \
						components/OTA \
						components/GUI \
						components/drivers/sd_storage \
						components/drivers/battery \
						components/emulators/GBC \
						components/emulators/GBC/gnuboy \
						components/emulators/SMS \
						components/emulators/SMS/smsplus \
						components/emulators/NES \
						components/emulators/NES/nofrendo \
						components/emulators/SNES/snes9x \
						


include $(IDF_PATH)/make/project.mk