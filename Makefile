BOARD_PORT?=/dev/cu.usbmodem14101

deps:
	brew install arduino-cli
	arduino-cli core update-index
	arduino-cli core install arduino:avr

build:
	arduino-cli compile --fqbn arduino:avr:micro --verbose SID.ino

check-board:
	arduino-cli board list | grep "$(BOARD_PORT)"

upload: build check-board
	arduino-cli upload --port "$(BOARD_PORT)" --fqbn arduino:avr:micro --verbose SID.ino

format:
	astyle --style=google --indent=spaces=2 SID.ino

.PHONY: build check-board deps format upload verify
