BOARD_PORT?=/dev/cu.usbmodem14101

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

BUILD_PROPERTIES=$(shell arduino-cli compile --fqbn arduino:avr:micro --show-properties | grep 'compiler.cpp.flags=' | sed 's/fpermissive/fno-permissive/; s/{compiler.warning_flags}/-Wall -Wextra -Wpedantic/; s/std=gnu++11/std=gnu++17/')

build:
	arduino-cli compile --fqbn arduino:avr:micro --verbose --build-properties "compiler.warning_flags=-Wpedantic,$(BUILD_PROPERTIES)" SID.ino

upload: check-board build
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.ino

format:
	clang-format -i SID.ino

test/runner: test/test.c
	clang test/test.c -o test/runner

test: test/runner
	./test/runner

.PHONY: build check-board deps format test upload verify
