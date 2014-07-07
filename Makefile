ARDUINO_DIR?=$(HOME)/Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR=$(CURDIR)/Arduino-Makefile
ARDMK_PATH=$(CURDIR)/Arduino-Makefile/bin
ARDUINO_LIBS=
BOARD_TAG?=atmega328
MONITOR_PORT?=/dev/tty.usbserial-A600afNY

include Arduino-Makefile/Arduino.mk


.DEFAULT: upload
