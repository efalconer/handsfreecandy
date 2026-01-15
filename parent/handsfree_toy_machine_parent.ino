#include <Wire.h>
#include <Servo.h>

Servo myservo;  // create servo object to control a servo

const int BUTTONPIN = 2;     // the number of the pushbutton pin
const int SENSORPIN = 4;
const int CHILD_DEVICE = 9;
const int REVERSEBUTTONPIN = 7;

const int MESSAGE_IDLE = 0;
const int MESSAGE_RUNNING = 1;
const int MESSAGE_DISPENSED = 2;
const int MESSAGE_REVERSE = 3;

const int REACTION_BUFFER_MS = 500;
const int MANUAL_COOLDOWN_MS = 5000;
const int COOLDOWN_MS = 6000;


// variables will change:
int buttonState = 0;         // variable for reading the pushbutton status
int reverseButtonState = 0;
int servoPin = 10;
int servoRotate = 115;
int servoReverseRotate = 80;
bool servoRunning = false;
bool servoReversing = false;
int sensorState = 0;
bool buttonReleased = true;
bool readyToVend = true;

unsigned long buttonPressedTime = 0;
unsigned long buttonReleasedTime = 0;
unsigned long lastToyDispensedTime = 0;

bool childBoardAvailable = true; // change to true before final


void sendMessage(int message) {
  // only send it if we can
  if (childBoardAvailable) {
    Wire.beginTransmission(CHILD_DEVICE); 
    Wire.write(message);
    Wire.endTransmission();
  } else {
    Serial.println("Could not send message");
  }
}

void setup() {
  // Start the I2C Bus as Master
  Wire.begin(); 
  
  // initialize the pushbutton pin as an input:
  pinMode(BUTTONPIN, INPUT);
  pinMode(REVERSEBUTTONPIN, INPUT);

  // setup the sensor
  pinMode(SENSORPIN, INPUT);     
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup

  lastToyDispensedTime = millis();

  Serial.begin(115200);
  Serial.println("Setup complete");
}

void runUntilSensorClear(int timeout) {
  int start = millis();
  sensorState = digitalRead(SENSORPIN);
  while (sensorState == LOW && millis()-start < timeout) {
    delay(50);
    sensorState = digitalRead(SENSORPIN);
  }

  if (sensorState == HIGH) {
    Serial.println("Sensor clear");
  } else { 
    Serial.println("Sensor timeout");
  }

  delay(1000);
}


void loop() {
  // read the state of the pushbutton value:
  buttonState = digitalRead(BUTTONPIN);
  reverseButtonState = digitalRead(REVERSEBUTTONPIN);
  sensorState = digitalRead(SENSORPIN);

  if (reverseButtonState == HIGH && !servoReversing) {
    servoRunning = false;
    servoReversing = true;
    readyToVend = false;
    Serial.println("Reverse pressed");
    sendMessage(MESSAGE_REVERSE);
    myservo.attach(servoPin);
    myservo.write(servoReverseRotate);
    delay(10);
  } else if (reverseButtonState == LOW && servoReversing) {
    servoReversing = false;
    Serial.println("Stopping reverse");
    myservo.detach();
    lastToyDispensedTime = millis();
    sendMessage(MESSAGE_DISPENSED);
    delay(10);
  } else if (reverseButtonState == HIGH) {
    // skip other event processing
    return;
  }

  // check if we're ready to dispense
  if (!servoReversing
      && !readyToVend 
      && !servoRunning 
      && millis() - lastToyDispensedTime > COOLDOWN_MS) {
    Serial.println("Ready to vend");
    readyToVend = true;
    sendMessage(MESSAGE_IDLE);
  }

  

  // check if the pushbutton is pressed.
  // if it is, the buttonState is LOW:
  // also don't dispense if switch has been released from previous time
  if (buttonState == LOW && !servoRunning && buttonReleased
      && millis() - lastToyDispensedTime > COOLDOWN_MS) {
    // turn motor
    Serial.println("Dispense toy");
    servoRunning = true;
    buttonReleased = false;
    buttonPressedTime = millis();
    myservo.attach(servoPin);
    myservo.write(servoRotate);
    servoRunning = true;
    readyToVend = false;
    buttonReleased = false;
    buttonPressedTime = millis();
    sendMessage(MESSAGE_RUNNING); // tell child board we're running
  } else if (servoRunning && buttonState == LOW 
            && buttonReleased 
            && millis() - buttonReleasedTime > MANUAL_COOLDOWN_MS) {
      Serial.println("Manual stop");
      myservo.detach();
      servoRunning = false;
      buttonReleased = false;
      lastToyDispensedTime = millis();
      sendMessage(MESSAGE_IDLE); // tell child board we're running
  }

  if (!buttonReleased && buttonState == HIGH) {
    if (millis() - buttonPressedTime > REACTION_BUFFER_MS) {
      Serial.println("Button released");
      buttonReleased = true;
      buttonReleasedTime = millis();
      delay(25); // avoid overlapping signals
    }
  }

  
  if (servoRunning && sensorState == LOW) { 
        Serial.println("Toy dispensed");
        runUntilSensorClear(3000);
        myservo.detach();
        servoRunning = false;
        lastToyDispensedTime = millis();
        sendMessage(MESSAGE_DISPENSED); // tell child board we're running
  }
}
