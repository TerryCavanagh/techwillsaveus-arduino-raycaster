#ifndef Gamer_h
#define Gamer_h

#include "Arduino.h"
#include <avr/interrupt.h>
#include <avr/io.h>

class Gamer {
public:
	Gamer();
	
	// Keywords
	#define UP 0
	#define LEFT 1
	#define RIGHT 2
	#define DOWN 3
	#define START 4
	#define LDR 5
	
	// Setup
	void begin();
	void update();
	
	// Inputs
	bool isPressed(uint8_t input);
	bool isHeld(uint8_t input);
	int ldrValue();
	void setldrThreshold(uint16_t threshold);
	
	// Outputs
	void setRefreshRate(uint16_t refreshRate);
	void updateDisplay();
	void allOn();
	void clear();
	void printImage(byte* img);
	void setLED(bool value);
	void toggleLED();
	
	// Variables
	byte display[8][8];
	byte pulseCount;
	byte buzzerCount;
	byte nextRow;
	byte currentRow;
	byte counter;
	byte image[8];
	
	void isrRoutine();
	
private:
	
	// Keywords
	#define CLK1 6
	#define DAT 8
	#define LAT 9
	#define CLK2 7
	#define DAT 8
	#define LAT 9
	#define OE 10
	#define LED 13
	#define BUZZER 2
	#define RX 5
	#define TX 4
	
	#define DEBOUNCETIME 50
	
	// Variables
	uint16_t _refreshRate;
	bool buttonFlags[6];
	unsigned long buttonLastPressed[6];
	int lastInputState[6];
	uint16_t ldrThreshold;
	
	// Functions
	void writeToDriver(byte dataOut);
	void writeToRegister(byte dataOut);
	void checkSerial();
	void checkInputs();
	void updateRow();
	
};

#endif

/*
TO-DO
-Fix IR
-Buzzer play melody
-LDR as button event
-Scrolling text
-Gamer clear with no update
*/


Gamer *thisGamer = NULL;

// Interrupt service routine
ISR(TIMER2_COMPB_vect) {
	thisGamer->isrRoutine();
}

Gamer::Gamer() {
}

// Setup inputs, outputs, timers, etc. Call this from setup()!!
void Gamer::begin() {
	::thisGamer = this;
	_refreshRate = 50;
	ldrThreshold = 300;
	
	// Setup outputs
	pinMode(3, OUTPUT);
	for(int i=6; i<=10; i++) pinMode(i, OUTPUT);
	pinMode(2, OUTPUT);
	pinMode(13, OUTPUT);
	
	// Setup inputs
	DDRC = B00000;
	PORTC = B11111;
	
	// Timer setup
	TCCR2A = _BV(COM2B1) | _BV(WGM21) | _BV(WGM20);
	TCCR2B = _BV(WGM22) | _BV(CS22);
	OCR2A = 51;
	OCR2B = 26;
	TCCR2B = (TCCR2B & 0b00111000) | 0x2;
	TIMSK2 = _BV(OCIE2B);
}

// Inputs --------------------------------

// Returns true if a button has been pressed recently
bool Gamer::isPressed(uint8_t input) {
	if(buttonFlags[input]) {
		buttonFlags[input] = 0;
		return true;
	}
	else return false;
}

// Returns true if the button is currently held down
bool Gamer::isHeld(uint8_t input) {
	bool result = (PINC & (1<<input)) >> input;
	return !result;
}

// Returns the current value of the LDR
int Gamer::ldrValue() {
	return analogRead(LDR);
}

// Change the button threshold for the LDR.
void Gamer::setldrThreshold(uint16_t threshold) {
	ldrThreshold = threshold;
}

// Outputs -------------------------------

// Set the display's refresh rate. 1 = 1 row per timer cycle. 10 = 1 row every 10 timer cycles
void Gamer::setRefreshRate(uint16_t refreshRate) {
	_refreshRate = refreshRate;
}

// Burns the display[][] array onto the display. Only call when you're done changing pixels!
void Gamer::updateDisplay() {
	byte newImage[8];
	for(int j=0; j<8; j++) {
		newImage[j] = 0x00;
		for(int i=0; i<8; i++) {
			newImage[j] <<= 1;
			newImage[j] |= display[i][j];
		}
	}
	if(newImage != image) {
		for(int i=0; i<8; i++) image[i] = newImage[i];
	}
}

// Turn on all pixels on display
void Gamer::allOn() {
	for(int j=0; j<8; j++) {
		for(int i=0; i<8; i++) display[i][j] = 1;
	}
	updateDisplay();
}

void Gamer::clear() {
	for(int j=0; j<8; j++) {
		for(int i=0; i<8; i++) display[i][j] = 0;
	}
	updateDisplay();
}

// Print an 8 byte array onto the display
void Gamer::printImage(byte* img) {
	for(int j=0; j<8; j++) {
		for(int i=0; i<8; i++) {
			display[i][j] = (img[j] & (1 << (7-i))) != 0;
		}
	}
	updateDisplay();
}

// Set the value of the Gamer LED
void Gamer::setLED(bool value) {
	digitalWrite(LED, value);
}

// Toggle the Gamer LED
void Gamer::toggleLED() {
	digitalWrite(LED, !digitalRead(LED));
}

// Internal display refreshing and writing to ICs ----------------------

// Load the next row in the display.
void Gamer::updateRow() {
	if(counter==8) {
		counter = 0;
		currentRow = 0x80;
	}
	writeToRegister(0);
	writeToDriver(image[counter]);
	writeToRegister(currentRow);
	currentRow >>= 1;
	counter++;
}

// Writes to the TLC5916 LED driver (cathodes)
void Gamer::writeToDriver(byte dataOut) {
	digitalWrite(OE, HIGH);
	
	for(int x=0; x<=7; x++) {
    	digitalWrite(CLK1, LOW);
    	digitalWrite(DAT, (dataOut & (1<<x)) >> x);
    	digitalWrite(CLK1, HIGH);
  	}

	digitalWrite(LAT, HIGH);
	digitalWrite(LAT, LOW);
	digitalWrite(OE, LOW);
}

// Write to the MIC5891 shift register (anodes)
void Gamer::writeToRegister(byte dataOut) {
	digitalWrite(LAT, LOW);
	
	for(int y=0; y<=7; y++) {
		digitalWrite(DAT, (dataOut & (1<<y)) >> y);
		digitalWrite(CLK2, HIGH);
		digitalWrite(CLK2, LOW);
	}
	digitalWrite(LAT, HIGH);
	digitalWrite(LAT, LOW);
}

// Periodically check if inputs are pressed (+ debouncing)
void Gamer::checkInputs() {
	int currentInputState[6];
	for(int i=0; i<6; i++) {
		if(i != 5) {
			currentInputState[i] = (PINC & (1<<i)) >> i;
			if(currentInputState[i] != lastInputState[i]) {
				if(currentInputState[i] == 0) {
					buttonFlags[i] = 1;
				}
			}
			lastInputState[i] = currentInputState[i];
		}
		else {
			currentInputState[i] = analogRead(LDR);
			if(currentInputState[i] - lastInputState[i] >= ldrThreshold) buttonFlags[i] = 1;
			lastInputState[i] = currentInputState[i];
		}
	}
}

// Run Interrupt Service Routine tasks
void Gamer::isrRoutine() {
	buzzerCount++;
	pulseCount++;
	if(pulseCount >= _refreshRate) {
		updateRow();
		pulseCount = 0;
	}
	if(pulseCount == _refreshRate/2) {
		checkInputs();
	}
}

//Custom helper functions!

Gamer gamer;

int screen[] = {
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0,
  0,0,0,0,0,0,0,0};

int x, y;

void updatescreen(){
  for(y = 0; y < 8; y++){              //start copying pixels on y
    for(x = 0 ; x < 8; x++){              //and x axis
      gamer.display[x][y] = screen[x + y*8];
    }
  }
  gamer.updateDisplay();
}

void cls(){
  for(y = 0; y < 8; y++){              //start copying pixels on y
    for(x = 0 ; x < 8; x++){              //and x axis
      screen[x + y*8] = 0;
    }
  }
}

void point(int _x, int _y, int _b){
  if(_x>=0 && _x<8 && _y>=0 && _y<8){
    screen[_x + _y*8] = _b;
  }
}


void line(int x,int y,int x2, int y2, int color) {
    int w = x2 - x ;
    int h = y2 - y ;
    int dx1 = 0, dy1 = 0, dx2 = 0, dy2 = 0 ;
    if (w<0) dx1 = -1 ; else if (w>0) dx1 = 1 ;
    if (h<0) dy1 = -1 ; else if (h>0) dy1 = 1 ;
    if (w<0) dx2 = -1 ; else if (w>0) dx2 = 1 ;
    int longest = w;
    int shortest = h;
    
    if(longest<0) longest = -longest;
    if(shortest<0) shortest = -shortest;
    
    if (!(longest>shortest)) {
        longest = h;
        shortest = w;
        
        if(longest<0) longest = -longest;
        if(shortest<0) shortest = -shortest;
        if (h<0) dy2 = -1 ; else if (h>0) dy2 = 1 ;
        dx2 = 0 ;            
    }
    int numerator = longest >> 1 ;
    for (int i=0;i<=longest;i++) {
        point(x,y,color) ;
        numerator += shortest ;
        if (!(numerator<longest)) {
            numerator -= longest ;
            x += dx1 ;
            y += dy1 ;
        } else {
            x += dx2 ;
            y += dy2 ;
        }
    }
}

void rect(int x1, int y1, int x2, int y2, int color){
  line(x1, y1, x2,y1, color);
  line(x2, y1, x2,y2, color);
  line(x1, y2, x2,y2, color);
  line(x1, y1, x1,y2, color);
}

void fillrect(int x1, int y1, int x2, int y2, int color){
  if(x1>x2){ x=x1; x1 = x2; x2 = x; }
  if(y1>y2){ y=y1; y1 = y2; y2 = y; }
  
  for(y=y1; y<=y2; y++){
    for(x=x1; x<=x2; x++){
      point(x,y,color);
    }
  }
}

int buzzercount=0;

void beep(int t){
  buzzercount=t;
}

void libupdate(){
  //Make a beep
  if(buzzercount>0){
    buzzercount--;
    
    digitalWrite(BUZZER, HIGH);
  }
}

int refreshrate;

