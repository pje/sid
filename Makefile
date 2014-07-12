ARDUINO_DIR?=$(HOME)/Applications/Arduino.app/Contents/Resources/Java
ARDMK_DIR=$(CURDIR)/Arduino-Makefile
ARDMK_PATH=$(CURDIR)/Arduino-Makefile/bin
ARDUINO_LIBS=
BOARD_TAG?=micro

include Arduino-Makefile/Arduino.mk

.DEFAULT: upload
