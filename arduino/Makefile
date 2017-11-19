# ----------------------------------------- #
# Global system paths                       #
# ----------------------------------------- #
export AVR_TOOLS_DIR=/usr
export ARDMK_DIR=/usr/share/arduino
export ARDUINO_DIR=/home/igor/local/src/arduino-1.8.2

# ----------------------------------------- #
# Project specific setup                    #
# ----------------------------------------- #
BOARD_TAG    = leonardo
MONITOR_PORT = /dev/ttyACM0
ARDUINO_lIBS = Adafruit_NeoPixel Timer ClickEncoder TimerOne

export CPPFLAGS+=-std=c++14 -fno-threadsafe-statics

include $(ARDMK_DIR)/Arduino.mk

check-syntax:	
	$(CXX) -c -include Arduino.h   -x c++ $(CXXFLAGS)   $(CPPFLAGS)  -fsyntax-only $(CHK_SOURCES)

