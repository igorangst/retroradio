#include <Adafruit_NeoPixel.h>
#include <Timer.h>
#include <ClickEncoder.h>
#include <TimerOne.h>

// ============================================== Global State
enum State {OFF, UP, ON, DOWN};     // Global system state
State state = OFF;                  // Starting in OFF state

const unsigned T_SHUTDOWN = 20000;  // ms for shutdown (20s)
unsigned long shutDownTime;         // Time elapsed since shutdown called

bool msg_ready = false;             // Incoming serial message pending
String msg;                         // Serial message
bool piIsUp = false;                // Status of Raspberry Pi

// ============================================== Pins
const unsigned powerButton = 9;   // Power on/off
const unsigned relaisPin   = 10;  // Relais control
const unsigned ledPin      = 14;  // RGB LED data in
const unsigned colPotPin   = A0;  // Colour select potentiometer

// ============================================== Rotary encoders
const unsigned leftButton  = 6;   // Left rotary encoder button
const unsigned leftRotA    = 2;   // Phase A
const unsigned leftRotB    = 4;   // Phase B
int16_t        leftDelta   = 0;   // Relavive rotation amount

const unsigned rightButton = 7;   // Right rotary encoder button
const unsigned rightRotA   = 3;   // Phase A
const unsigned rightRotB   = 5;   // Phase B
int16_t        rightDelta  = 0;   // Relavive rotation amount

ClickEncoder *leftEncoder, *rightEncoder;

void timerIsr() {
  leftEncoder->service();
  rightEncoder->service();
}

// ============================================== RGB LED
Adafruit_NeoPixel led = Adafruit_NeoPixel(1, ledPin, NEO_GRB + NEO_KHZ800);

volatile int fader = 180;     // Value of fader potentiometer

Timer t;                      // Timer for light control      
unsigned tick = 0;            // Ticks once every T_LIGHT ms
const unsigned T_LIGHT = 50;  // Interval (ms) for light control

#define MAXBRIGHT 127         // Maximum brightness level for RGB LED
#define SECTOR 60             // Degrees per sector in color scheme

int fadeIn(int deg)
{
  return (deg * MAXBRIGHT) / SECTOR;
}

int fadeOut(int deg)
{
  return ((SECTOR - deg) * MAXBRIGHT) / SECTOR;
}

// Calculate RGB values of a rainbow streteched over 360 degrees
void rainbow(const int deg, int *r, int *g, int *b)
{
  if (deg < SECTOR){
    *r = fadeIn(deg);
    *g = 0;
    *b = 0;
  } else if (deg < 2*SECTOR){
    *r = MAXBRIGHT;
    *g = fadeIn(deg - SECTOR);
    *b = 0;
  } else if (deg < 3*SECTOR){
    *r = fadeOut(deg - 2*SECTOR);
    *g = MAXBRIGHT;
    *b = 0;
  } else if (deg < 4*SECTOR){
    *r = 0;
    *g = MAXBRIGHT;
    *b = fadeIn(deg - 3*SECTOR);
  } else if (deg < 5*SECTOR){
    *r = 0;
    *g = fadeOut(deg - 4*SECTOR);
    *b = MAXBRIGHT;    
  } else {
    *r = fadeIn(deg - 5*SECTOR);
    *g = fadeIn(deg - 5*SECTOR);
    *b = MAXBRIGHT;
  }
}

uint32_t wheel(int angle){
  int r, g, b;
  rainbow(angle, &r, &g, &b);
  return led.Color(r, g, b);
}

void light() {
  switch (state) {
  case OFF: 

    // Light off
    led.setPixelColor(0, led.Color(0,0,0));
    led.show();
    break;
  case UP:

    // Flashing green
    tick = (tick + 1) % 20;
    if (tick < 10) {
      led.setPixelColor(0, led.Color(127, 0, 0));
    } else {
      led.setPixelColor(0, led.Color(0,0,0));
    }
    led.show();
    break;
  case DOWN:

    // Flashing red
    tick = (tick + 1) % 20;
    if (tick < 10) {
      led.setPixelColor(0, led.Color(0, 127, 0));
    } else {
      led.setPixelColor(0, led.Color(0,0,0));
    }
    led.show();
    break;
  case ON:

    // Set color using fader poti
    int pot = analogRead(colPotPin);
    int angle = map(pot, 0, 1023, 0, 359);
    // uint32_t color = wheel(angle);
    uint32_t color = wheel(fader);
    led.setPixelColor(0, color);
    led.show();
  }  
}

// ============================================== Behavior
void stateOn() {
  
  leftDelta += leftEncoder->getValue();
  if (leftDelta > 0) {
    Serial.println("R+");
    leftDelta = 0;  
  } else if (leftDelta < 0) {
    Serial.println("R-");
    leftDelta = 0;      
  }
  
  rightDelta += rightEncoder->getValue();
  if (rightDelta > 0) {
    Serial.println("M+");
    rightDelta = 0;  
  } else if (rightDelta < 0) {
    Serial.println("M-");
    rightDelta = 0;      
  }

  ClickEncoder::Button lButton = leftEncoder->getButton();
  if (lButton == ClickEncoder::Clicked){
    Serial.println("R");        
  }
  ClickEncoder::Button rButton = rightEncoder->getButton();
  if (rButton == ClickEncoder::Clicked){
    Serial.println("M");        
  }
  
  bool power = !digitalRead(powerButton);
  if (!power){

    // Send shutdown message to Pi
    state = DOWN;
    shutDownTime = millis();
    Serial.println("DOWN");
  }

  // Check for incoming serial message
  if (msg_ready){
    if (msg == "DOWN"){
      state = DOWN;
      shutDownTime = millis();
    }
    msg = "";
    msg_ready = false;
  }
}

void stateOff() {
  bool button = !digitalRead(powerButton);
  if (button){

    // Power up Pi
    state = UP;
    piIsUp = false;
    digitalWrite(relaisPin, HIGH);
    Serial.println("UP");
  }
}

void stateUp() {
  
  // Check for incoming serial message
  if (msg_ready){
    if (msg == "UP"){
      state = ON;
    }
    Serial.println(msg);
    msg = "";
    msg_ready = false;
  }
}

void stateDown() {
  long elapsed = millis() - shutDownTime;
  if (elapsed >= T_SHUTDOWN){

    // Power off Pi
    state = OFF;
    digitalWrite(relaisPin, LOW);
    Serial.println("OFF");
  }  
}


// ================================================= SETUP

void setup() {
  Serial.begin(9600);
  
  led.begin();
  led.show();

  pinMode(leftButton, INPUT_PULLUP);
  pinMode(leftRotA, INPUT_PULLUP);
  pinMode(leftRotB, INPUT_PULLUP);
  pinMode(rightButton, INPUT_PULLUP);
  pinMode(rightRotA, INPUT_PULLUP);
  pinMode(rightRotB, INPUT_PULLUP);
  pinMode(powerButton, INPUT_PULLUP);
  pinMode(relaisPin, OUTPUT);

  // Close relais
  digitalWrite(relaisPin, LOW);

  // Set up rotary encoders
  leftEncoder = new ClickEncoder(leftRotA, leftRotB, leftButton);  
  rightEncoder = new ClickEncoder(rightRotA, rightRotB, rightButton);  
  Timer1.initialize(1000);
  Timer1.attachInterrupt(timerIsr); 

  // Start light control timer
  t.every(T_LIGHT, light);

  // initialize global state
  state = OFF;
}

// =================================== LOOP

void loop() {
  t.update();

  while (!msg_ready && Serial.available() > 0){
    int c = Serial.read();
    Serial.println(c);
    if (c == '\n'){
      msg_ready = true;
    } else {
      msg += Serial.read();
    }
  }

  // State machine
  if (state == ON){
    stateOn();
  } else if (state == OFF){
    stateOff();
  } else if (state == UP){
    stateUp();
  } else if (state == DOWN){
    stateDown();
  }

  delay(5);
}

// ============================================== Serial Event

void serialEvent() {
  while (!msg_ready && Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      msg_ready = true;
      Serial.print("msg = ");
      Serial.println(msg);
    } else {
      msg += inChar;
    }
  }
}
