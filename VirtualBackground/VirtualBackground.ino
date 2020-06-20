#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include "fauxmoESP.h"

// Servo definitions
#define RAISE_RIGHT 180
#define RAISE_LEFT 0
#define STOP 93
#define LOWER_RIGHT 0
#define LOWER_LEFT 180

// Height Definitions
#define NUMBER_ROTATIONS 30
#define COUNTS_PER_ROTATION 40

// Servo Pins
#define RIGHT_SERVO_PIN 13
#define LEFT_SERVO_PIN 12

// Rotary Encoder Pins
#define RIGHT_ROTARY_PIN_A 5
#define RIGHT_ROTARY_PIN_B 4
#define LEFT_ROTARY_PIN_A 0
#define LEFT_ROTARY_PIN_B 2

#define SERIAL_BAUDRATE 115200

// Wifi credentials
#define WIFI_SSID "*****"
#define WIFI_PASS "*****"

// The name that Alexa will see
#define BACKGROUND "Virtual Background"
fauxmoESP fauxmo;

// Keeps track of the background state (start with it off)
bool backgroundOn = false;
// Used to determine if we want to change the state (start with it false)
bool changeBackgroundState = false;

static const int MAX_HEIGHT = COUNTS_PER_ROTATION * NUMBER_ROTATIONS;
int left_currentHeight = 0;
int right_currentHeight = 0;


void raiseBackground();
void lowerBackground();
void updateHeight();


Servo rightServo;
Servo leftServo;

int right_aState, left_aState;
int right_aLastState, left_aLastState;

// Wi-Fi Connection
void wifiSetup() {
  // Set WIFI module to STA mode
  WiFi.mode(WIFI_STA);

  Serial.printf("[WIFI] Connecting to %s ", WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  // Wait for the wifi to connect
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(100);
  }
  Serial.println();
  Serial.printf("[WIFI] STATION Mode, SSID: %s, IP address: %s\n", WiFi.SSID().c_str(), WiFi.localIP().toString().c_str());
}

void setup() {
  // Init serial port and clean garbage
  Serial.begin(SERIAL_BAUDRATE);
  Serial.println();

  // Start wifi setup
  wifiSetup();

  pinMode(RIGHT_ROTARY_PIN_A, INPUT);
  pinMode(LEFT_ROTARY_PIN_A, INPUT);

  right_aLastState = digitalRead(RIGHT_ROTARY_PIN_A);
  left_aLastState = digitalRead(LEFT_ROTARY_PIN_A);

  // Setup the servo pins for outputs
  rightServo.attach(RIGHT_SERVO_PIN);
  leftServo.attach(LEFT_SERVO_PIN);

  // Start with the servos off
  rightServo.write(STOP);
  leftServo.write(STOP);

  /*  By default, fauxmoESP creates it's own webserver on the defined port
   *  The TCP port must be 80 for gen3 devices (default is 1901)
   *  This has to be done before the call to enable()
   */
  fauxmo.createServer(true); // not needed, this is the default value
  fauxmo.setPort(80); // This is required for gen3 devices

  /*  You have to call enable(true) once you have a WiFi connection
   *  You can enable or disable the library at any moment
   *  Disabling it will prevent the devices from being discovered and switched
   */
  fauxmo.enable(true);

  // Add virtual devices
  fauxmo.addDevice(BACKGROUND);

  // state: true turn on background, false turn off background
  fauxmo.onSetState([](unsigned char device_id, const char * device_name, bool state, unsigned char value) {
    Serial.printf("Value: %d\n", value);
    Serial.printf("[MAIN] Device #%d (%s) state: %s value: %d\n", device_id, device_name, state ? "ON" : "OFF", value);

    // change background state if the request is not the current state
    changeBackgroundState = state != backgroundOn;
  });
}

void loop() {
  /* IMPORTANT: This is an asynchronous function so anything with a delay needs to be handled outside of that function
   *  fauxmoESP uses an async TCP server but a sync UDP server
   *  Therefore, we have to manually poll for UDP packets
   */
  fauxmo.handle();

  // If background needs to be changed and it is on, turn it off
  if (changeBackgroundState && backgroundOn) {
    raiseBackground();
  } else if (changeBackgroundState && !backgroundOn) {
    lowerBackground();
  }
}


void raiseBackground() {
  Serial.println("Raising the background");
  while (left_currentHeight != 0 && right_currentHeight != 0) {
    if (left_currentHeight > 0) {
      leftServo.write(RAISE_LEFT);
    }
    if (right_currentHeight > 0) {
      rightServo.write(RAISE_RIGHT);
    }
    updateHeight();
  }

  rightServo.write(STOP);
  leftServo.write(STOP);
  changeBackgroundState = false;
  backgroundOn = false;
}


void lowerBackground() {
  Serial.println("Lowering the background");
  // Keep lowering the background until the current height is the max height
  while (left_currentHeight != MAX_HEIGHT && right_currentHeight != MAX_HEIGHT) {
    if (left_currentHeight < MAX_HEIGHT) {
      leftServo.write(LOWER_LEFT);
    }
    if (right_currentHeight < MAX_HEIGHT) {
      rightServo.write(LOWER_RIGHT);
    }
    updateHeight();
  }

  rightServo.write(STOP);
  leftServo.write(STOP);
  changeBackgroundState = false;
  backgroundOn = true;
}


void updateHeight() {
  right_aState = digitalRead(RIGHT_ROTARY_PIN_A);
  left_aState = digitalRead(LEFT_ROTARY_PIN_A);

  if (right_aState != right_aLastState) {
    if (digitalRead(RIGHT_ROTARY_PIN_B) != right_aState) {
      right_currentHeight++;
    } else {
      right_currentHeight--;
    }
  }

  if (left_aState != left_aLastState) {
    if (digitalRead(LEFT_ROTARY_PIN_B) != left_aState) {
      left_currentHeight++;
    } else {
      left_currentHeight--;
    }
  }

  right_aLastState = right_aState;
  left_aLastState = left_aState;
}
