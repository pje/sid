BOARD_PORT?=/dev/cu.usbmodemC1
ARDUINO_HARDWARE_DIR?=~/Library/Arduino15/packages/arduino/hardware/avr/1.8.3

BUILD_PROPERTIES=$(shell arduino-cli compile --fqbn arduino:avr:micro --show-properties | grep 'compiler.cpp.flags=' | sed 's/fpermissive/fno-permissive/; s/{compiler.warning_flags}/-Wall -Wextra -Wpedantic/; s/std=gnu++11/std=gnu++17/')
SOURCES=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)

deps: $(ARDUINO_HARDWARE_DIR)/variants/micro_norxled/pins_arduino.h $(ARDUINO_HARDWARE_DIR)/boards.local.txt
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI

$(ARDUINO_HARDWARE_DIR)/variants/micro_norxled/pins_arduino.h: $(CURDIR)/config/arduino_overrides/variants/micro_norxled/pins_arduino.h
	mkdir -p $(ARDUINO_HARDWARE_DIR)/variants/micro_norxled/
	cp $< $@

$(ARDUINO_HARDWARE_DIR)/boards.local.txt: $(CURDIR)/config/arduino_overrides/boards.local.txt
	cp $< $@

build: $(ARDUINO_HARDWARE_DIR)/boards.local.txt $(ARDUINO_HARDWARE_DIR)/variants/micro_norxled/pins_arduino.h
	arduino-cli compile --fqbn arduino:avr:micro --verbose --build-properties "compiler.warning_flags=-Wpedantic,$(BUILD_PROPERTIES)" SID.ino

upload: build
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.ino

format:
	clang-format -i SID.ino $(SOURCES) $(HEADERS)

TEST_SOURCES=$(wildcard test/*.c)
TEST_RUNNERS=$(patsubst %test.c,%test,$(TEST_SOURCES))

$(TEST_RUNNERS) : % : %.c
	clang -rdynamic -std=c11 -Wall -Wextra --debug -g3 $< -o $@
	chmod +x $@

test: $(TEST_RUNNERS)
	set -e; $(foreach runner,$(TEST_RUNNERS),./$(runner);)

.PHONY: build check-board config-overrides deps format test upload verify
