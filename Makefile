ARDUINO_DIR?=$(HOME)/Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR=$(CURDIR)/Arduino-Makefile
ARDMK_PATH=$(CURDIR)/Arduino-Makefile/bin
ARDUINO_LIBS=
BOARD_TAG?=micro
MONITOR_PORT?=/dev/tty.usbmodem*

include Arduino-Makefile/Arduino.mk

format:
	astyle --style=google --indent=spaces=2 SID.ino

.DEFAULT: upload

.PHONY: format upload
