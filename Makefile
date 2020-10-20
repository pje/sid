BOARD_PORT?=/dev/cu.usbmodem14101

BUILD_PROPERTIES=$(shell arduino-cli compile --fqbn arduino:avr:micro --show-properties | grep 'compiler.cpp.flags=' | sed 's/fpermissive/fno-permissive/; s/{compiler.warning_flags}/-Wall -Wextra -Wpedantic/; s/std=gnu++11/std=gnu++17/')
SOURCES=$(wildcard *.c)
HEADERS=$(wildcard *.h)

%.o : %.c

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI
	which rc && rc -J .

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

build:
	arduino-cli compile --fqbn arduino:avr:micro --verbose --build-properties "compiler.warning_flags=-Wpedantic,$(BUILD_PROPERTIES)" SID.cpp

upload: check-board build
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.cpp

format:
	clang-format -i SID.cpp $(SOURCES) $(HEADERS)

test/runner: test/test.c $(SOURCES) $(HEADERS)
	clang -std=c11 -Wall -Wextra --debug -g3 -rdynamic $(SOURCES) test/test.c -o test/runner

test: test/runner
	./test/runner

.PHONY: build check-board deps format test upload verify
