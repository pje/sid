BOARD_PORT?=/dev/cu.usbmodem14101

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr
	arduino-cli lib install USBMIDI

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

upload: check-board
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.ino

format:
	clang-format -i SID.ino

test/runner: test/test.c
	clang test/test.c -o test/runner

test: test/runner
	./test/runner

.PHONY: check-board deps format test upload verify
