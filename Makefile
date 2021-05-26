BOARD_PORT?=/dev/cu.usbmodemC1
ARDUINO_HARDWARE_DIR?=~/Library/Arduino15/packages/arduino/hardware/avr/1.8.3

BUILD_PROPERTIES=$(shell arduino-cli compile --fqbn arduino:avr:micro --show-properties | grep 'compiler.cpp.flags=' | sed 's/fpermissive/fno-permissive/; s/{compiler.warning_flags}/-Wall -Wextra -Wno-missing-field-initializers/; s/std=gnu++11/std=gnu++17/')
SOURCES=$(wildcard src/*.c)
HEADERS=$(wildcard src/*.h)

deps: $(ARDUINO_HARDWARE_DIR)/variants/micro_norxled/pins_arduino.h $(ARDUINO_HARDWARE_DIR)/boards.local.txt
	which arduino-cli || brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI
	[ -e ~/Documents/Arduino/libraries/MemoryFree ] || git clone https://github.com/McNeight/MemoryFree ~/Documents/Arduino/libraries/MemoryFree

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

clean:
	arduino-cli cache clean
	rm -rf $(TEST_RUNNERS)
	rm -rf test/*.dSYM
	rm -rf build
	rm -rf .clangd

TEST_SOURCES=$(wildcard test/*.c)
TEST_RUNNERS=test/deque_test test/hash_table_test test/util_test

test/deque_test: test/deque_test.c test/test_helper.h src/deque.h src/list_node.h src/note.h src/hash_table.h
	clang -std=c11 -Wall -Wextra -lm --debug -g3 test/deque_test.c -o $@
	chmod +x $@

test/hash_table_test: test/hash_table_test.c test/test_helper.h src/note.h src/hash_table.h src/list_node.h src/note.h
	clang -std=c11 -Wall -Wextra -lm --debug test/hash_table_test.c -o $@
	chmod +x $@

test/util_test: test/util_test.c test/test_helper.h src/util.h
	clang -std=c11 -Wall -Wextra -lm --debug test/util_test.c -o $@
	chmod +x $@

test: $(TEST_RUNNERS)
	set -e; $(foreach runner,$(TEST_RUNNERS),./$(runner);)

.PHONY: build check-board clean config-overrides deps format test upload verify
