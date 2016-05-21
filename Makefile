ARDUINO_BIN?=$(HOME)/Applications/Arduino.app/Contents/MacOS/Arduino
MONITOR_PORT?=/dev/tty.usbmodem*

upload:
	$(ARDUINO_BIN) --upload --board arduino:avr:micro --verbose SID.ino

verify:
	$(ARDUINO_BIN) --verify SID.ino

format:
	astyle --style=google --indent=spaces=2 SID.ino

# monitor serial port with:
#     screen /dev/ttyXXXX 9600
# (ctrl-a, k to kill)

.PHONY: format upload verify
