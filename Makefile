BOARD_PORT?=/dev/cu.usbmodemC1

BUILD_PROPERTIES=$(shell arduino-cli compile --fqbn arduino:avr:micro --show-properties | grep 'compiler.cpp.flags=' | sed 's/fpermissive/fno-permissive/; s/{compiler.warning_flags}/-Wall -Wextra -Wpedantic/; s/std=gnu++11/std=gnu++17/')
SOURCES=$(wildcard *.c)
HEADERS=$(wildcard *.h)

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI
	# arduino-cli lib install LibPrintf

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

build:
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

.PHONY: build check-board deps format test upload verify
