#include "Board.h"
#include "driver/rtc_io.h"

// Constructor
Board::Board()
	: loadCell(IN1MotorPin, IN2MotorPin, 300),
	motor(IN1MotorPin, IN2MotorPin),
	buttonUP(buttonUPpin, BUTTON_PULLDOWN, true, 50),
	buttonDOWN(buttonDOWNpin, BUTTON_PULLDOWN, true, 50) {}