BOARD_PORT?=/dev/cu.usbmodem14101

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI

build:
	arduino-cli compile --fqbn arduino:avr:micro --verbose SID.ino

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

upload: build check-board
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.ino

format:
	clang-format -i SID.ino

test/runner: test/test.c
	clang test/test.c -o test/runner

test: test/runner
	./test/runner

.PHONY: build check-board deps format test upload verify
