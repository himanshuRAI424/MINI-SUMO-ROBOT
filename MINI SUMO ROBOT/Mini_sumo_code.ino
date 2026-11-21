#include <Wire.h>
#include <VL53L0X.h>
#include <QTRSensors.h>

// ==========================================
// QTR SENSOR SETTINGS (EDGE DETECTION)
// ==========================================
QTRSensors qtr;
const uint8_t SensorCount = 2; 
uint16_t sensorValues[SensorCount];

const uint8_t SENSOR_1_PIN = 34; // Left Edge Sensor
const uint8_t SENSOR_8_PIN = 35; // Right Edge Sensor
const uint8_t EMITTER_PIN = 255; 
const uint8_t ESP32_LED = 2; 

// Detection threshold (0 = pure white, 1000 = pure black)
const int EDGE_THRESHOLD = 500; 

// ==========================================
// TOF SENSOR PINS & OBJECTS (ENEMY TRACKING)
// ==========================================
#define XSHUT_A 19 // Sensor A: Left 
#define XSHUT_B 17 // Sensor B: Front-Left 
#define XSHUT_C 16 // Sensor C: Front-Right 
#define XSHUT_D 18 // Sensor D: Right 

VL53L0X sensorLeft;       // A
VL53L0X sensorFrontLeft;  // B
VL53L0X sensorFrontRight; // C
VL53L0X sensorRight;      // D

// ==========================================
// MOTOR DRIVER PINS (TB6612FNG)
// ==========================================
const uint8_t PWMA = 32; // Speed Motor A (Left)
const uint8_t AIN1 = 25; // Dir 1 Motor A
const uint8_t AIN2 = 33; // Dir 2 Motor A

const uint8_t PWMB = 14; // Speed Motor B (Right)
const uint8_t BIN1 = 26; // Dir 1 Motor B
const uint8_t BIN2 = 27; // Dir 2 Motor B

const uint8_t STBY = 4;  // Standby pin

// ==========================================
// THRESHOLDS & SPEEDS
// ==========================================
const int ENEMY_DISTANCE = 220;      // mm (Front sensors)
const int ENEMY_DISTANCE_SIDE = 150; // mm (Side sensors)

const int BASE_SPEED = 230;          // Hunting speed
const int BASE_TURN = 230;           // Turning speed
const int ATTACK_SPEED = 250;        // Charging speed
const int CALIBRATION_SPEED = 170;   // Slower speed for spinning safely during calibration
const int BASE_SPEED_BACK = 230;

void setup()
{
  Serial.begin(115200);
  Wire.begin(); 

  // ------------------------------------------
  // 1. INITIALIZE TOF SENSORS
  // ------------------------------------------
  pinMode(XSHUT_A, OUTPUT);
  pinMode(XSHUT_B, OUTPUT);
  pinMode(XSHUT_C, OUTPUT);
  pinMode(XSHUT_D, OUTPUT);

  // Turn ALL sensors OFF initially to prevent I2C address conflicts
  digitalWrite(XSHUT_A, LOW);
  digitalWrite(XSHUT_B, LOW);
  digitalWrite(XSHUT_C, LOW);
  digitalWrite(XSHUT_D, LOW);
  delay(10);

  // Turn on and configure each sensor with a unique address
  digitalWrite(XSHUT_A, HIGH); delay(10);
  sensorLeft.init(); sensorLeft.setAddress(0x30); sensorLeft.setTimeout(500);

  digitalWrite(XSHUT_B, HIGH); delay(10);
  sensorFrontLeft.init(); sensorFrontLeft.setAddress(0x31); sensorFrontLeft.setTimeout(500);

  digitalWrite(XSHUT_C, HIGH); delay(10);
  sensorFrontRight.init(); sensorFrontRight.setAddress(0x32); sensorFrontRight.setTimeout(500);

  digitalWrite(XSHUT_D, HIGH); delay(10);
  sensorRight.init(); sensorRight.setAddress(0x33); sensorRight.setTimeout(500);

  // ------------------------------------------
  // 2. INITIALIZE QTR SENSORS
  // ------------------------------------------
  qtr.setTypeAnalog();
  qtr.setSensorPins((const uint8_t[]){SENSOR_1_PIN, SENSOR_8_PIN}, SensorCount);
  if (EMITTER_PIN != 255) { qtr.setEmitterPin(EMITTER_PIN); }

  // ------------------------------------------
  // 3. INITIALIZE MOTORS
  // ------------------------------------------
  pinMode(PWMA, OUTPUT); pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT);
  pinMode(PWMB, OUTPUT); pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  
  digitalWrite(STBY, HIGH); 
  
  delay(1000); // Give you a second to move your hands away before it spins
  
  pinMode(ESP32_LED, OUTPUT);
  digitalWrite(ESP32_LED, HIGH); // Calibration mode ON

  // ------------------------------------------
  // 4. AUTO-ROTATING CALIBRATION (EXTENDED TIME)
  // Make sure bot is on the white edge before this runs!
  // ------------------------------------------
  
  // Spin Right
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, CALIBRATION_SPEED);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); analogWrite(PWMB, CALIBRATION_SPEED);
  for (int i = 0; i < 150; i++) { qtr.calibrate(); delay(20); }

  // Spin Left
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); analogWrite(PWMA, CALIBRATION_SPEED);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); analogWrite(PWMB, CALIBRATION_SPEED);
  for (int i = 0; i < 150; i++) { qtr.calibrate(); delay(20); }

  stopMotors();
  digitalWrite(ESP32_LED, LOW); // Calibration done
  
  delay(1000); // Wait 1 second before the match actually starts
}

void loop()
{
  // 1. Read QTR edge sensors
  qtr.readCalibrated(sensorValues);
  bool leftDetectsEdge = sensorValues[0] < EDGE_THRESHOLD;
  bool rightDetectsEdge = sensorValues[1] < EDGE_THRESHOLD;

  // 2. SURVIVAL INSTINCT: Edge Detection Priority
  if (leftDetectsEdge && rightDetectsEdge) {

    moveBack();
    delay(150);
    uTurn();
  } 
  else if (leftDetectsEdge) {
    // Sensor 1 (Left) hit the edge -> Turn Right to escape
    moveBack();
    delay(150);
    turnRight();
    delay(350); 
  } 
  else if (rightDetectsEdge) {
    // Sensor 8 (Right) hit the edge -> Turn Left to escape
    moveBack();
    delay(150);
    turnLeft(); 
    delay(350); 
  } 
  else {
    // 3. ATTACK INSTINCT: Safe from edges -> Hunt the enemy!
    trackWithFourSensors();
  }
  
  // A tiny delay keeps the I2C bus and loop stable
  delay(10); 
}

// ==========================================
// 4-SENSOR ACTIVE TRACKING LOGIC
// ==========================================
void trackWithFourSensors() {
  int distLeft       = sensorLeft.readRangeSingleMillimeters();
  int distFrontLeft  = sensorFrontLeft.readRangeSingleMillimeters();
  int distFrontRight = sensorFrontRight.readRangeSingleMillimeters();
  int distRight      = sensorRight.readRangeSingleMillimeters();

  bool enemyOnFarLeft   = (distLeft < ENEMY_DISTANCE_SIDE);
  bool enemyOnFrontLeft = (distFrontLeft < ENEMY_DISTANCE);
  bool enemyOnFrontRight= (distFrontRight < ENEMY_DISTANCE);
  bool enemyOnFarRight  = (distRight < ENEMY_DISTANCE_SIDE);

  if (enemyOnFrontLeft && enemyOnFrontRight) {
    // Dead Center!
    Attack(); 
  } 
  else if (enemyOnFrontLeft) {
    turnLeft();
  } 
  else if (enemyOnFarLeft) {
    turnLeft();
    delay(230); 
  }
  else if (enemyOnFrontRight) {
    turnRight();
  } 
  else if (enemyOnFarRight) {
    turnRight();
    delay(230);
  }
  else {
    // Nobody sees the target -> Go Forward to hunt!
    moveForward();
  }
}

// ==========================================
// MOTOR CONTROL HELPER FUNCTIONS
// ==========================================

void moveForward() {
  // Left Wheel Forward
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, BASE_SPEED); 
  // Right Wheel Forward
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); analogWrite(PWMB, BASE_SPEED);
}

void moveBack() {
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); analogWrite(PWMA, BASE_SPEED_BACK); 
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); analogWrite(PWMB, BASE_SPEED_BACK);
}

void Attack() {
  // Left Wheel Forward Max
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, ATTACK_SPEED);
  // Right Wheel Forward Max
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); analogWrite(PWMB, ATTACK_SPEED);
}

void turnLeft() {
  // Left Wheel Backward
  digitalWrite(AIN1, HIGH);  digitalWrite(AIN2, LOW); analogWrite(PWMA, BASE_TURN);
  // Right Wheel Forward
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH);  analogWrite(PWMB, BASE_TURN);
}

void turnRight() {
  // Left Wheel Forward
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH);  analogWrite(PWMA, BASE_TURN);
  // Right Wheel Backward
  digitalWrite(BIN1, HIGH);  digitalWrite(BIN2, LOW); analogWrite(PWMB, BASE_TURN);
}

void uTurn() {
  // Spin right for a longer duration to do a U-turn
  turnRight();
  delay(400); // Adjust this delay until the bot turns roughly 180 degrees
}

void stopMotors() {
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}