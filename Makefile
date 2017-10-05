ARDUINO_BIN?=/Applications/Arduino.app/Contents/MacOS/Arduino
MONITOR_PORT?=/dev/tty.usbmodem*

build:

upload:
	$(ARDUINO_BIN) --upload --board arduino:avr:micro --verbose SID.ino

verify:
	$(ARDUINO_BIN) --verify SID.ino

format:
	astyle --style=google --indent=spaces=2 SID.ino

.PHONY: format upload verify
