/**
 * Motion Glasses - Assistive Navigation Firmware for Arduino
 * Author: Ahmad Serieh <ahmadserieh.official@gmail.com>
 * License: MIT
 *
 * Senses directional obstacles and ground topography using 3 ultrasonic sensors
 * and an MPU-6050 accelerometer/gyroscope, communicating directional feedback
 * through dual haptic vibration motors.
 */

#include <Wire.h>
#include <MPU6050.h>

// Distance detection threshold in centimeters
#define DISTANCE_THRESHOLD 100

// Maximum consecutive cycles allowed at identical distance before auto-muting vibration
#define CYCLE_LIMIT 46

// Maximum pulse wait time in microseconds (~5 meters range)
#define ULTRASONIC_TIMEOUT_US 30000

// Ultrasonic sensor pins (using 3-pin mode where TRIG and ECHO share the same pin)
const int SL_TRIG = 5;  // Left ultrasonic sensor pin
const int SR_TRIG = 6;  // Right ultrasonic sensor pin
const int SF_TRIG = 9;  // Front ultrasonic sensor pin

// Vibration motor driver pins
const int VL_PIN = 2;   // Left vibration motor
const int VR_PIN = 3;   // Right vibration motor

// Slide switch inputs
const int strPin = 4;   // Straight-mode override slide switch
const int noCyPin = 13; // Cycle-limit disable slide switch

// Ground surface baseline distance in centimeters
double level = 0.0;

// Head pitch angle threshold (degrees) below which down-mode activates
const int downAngle = 30;

// Consecutive identical reading cycle counters
int cycleF = 0;
int cycleR = 0;
int cycleL = 0;

// Previous distance readings
double preDF = 0.0;
double preDR = 0.0;
double preDL = 0.0;

// MPU6050 sensor instance
MPU6050 mpu;

// Function prototypes
long readUltrasonicDistance(int triggerPin, int echoPin);
void straightMode(double df, double dr, double dl);
void downMode(double df);
void setLevel();
void updateCycleCount(double df, double dr, double dl);

void setup()
{
  Serial.begin(9600);

  // Configure vibration motor pins
  pinMode(VL_PIN, OUTPUT);
  pinMode(VR_PIN, OUTPUT);
  digitalWrite(VL_PIN, LOW);
  digitalWrite(VR_PIN, LOW);

  // Configure switch pins
  pinMode(noCyPin, INPUT);
  pinMode(strPin, INPUT);

  // Initialize I2C communication and MPU6050
  Wire.begin();
  mpu.begin();
  mpu.calcOffsets(true, true);

  // Stabilization delay
  delay(1000);

  // Calibrate baseline ground surface distance
  setLevel();
}

void loop()
{
  // Read normalized acceleration and calculate pitch angle
  Vector normAccel = mpu.readNormalizeAccel();
  float pitch = -1.0 * asin(normAccel.XAxis) * 180.0 / PI;

  Serial.print(F("Pitch angle: "));
  Serial.println(pitch);

  // Measure distances from all three ultrasonic sensors
  double df = readUltrasonicDistance(SF_TRIG, SF_TRIG);
  double dr = readUltrasonicDistance(SR_TRIG, SR_TRIG);
  double dl = readUltrasonicDistance(SL_TRIG, SL_TRIG);

  Serial.print(F("Ultrasonic Distances (Front Right Left): "));
  Serial.print(df);
  Serial.print(F(" "));
  Serial.print(dr);
  Serial.print(F(" "));
  Serial.println(dl);

  // Check for sensor fault or inactive reading (0 cm indicates timeout / sleep)
  if (df == 0 || dr == 0 || dl == 0)
  {
    Serial.println(F("Sleep Mode is Active"));
    digitalWrite(VR_PIN, LOW);
    digitalWrite(VL_PIN, LOW);
  }
  else
  {
    // Mode selection: Straight navigation vs. ground tracking
    if (pitch > downAngle || digitalRead(strPin) == HIGH)
    {
      straightMode(df, dr, dl);
    }
    else
    {
      downMode(df);
    }

    // Update cycle counters unless cycle limiting is disabled
    if (digitalRead(noCyPin) == LOW)
    {
      updateCycleCount(df, dr, dl);
    }
    else
    {
      cycleF = 0;
      cycleR = 0;
      cycleL = 0;
    }

    Serial.print(F("Cycle Counts: "));
    Serial.print(cycleF);
    Serial.print(F(" "));
    Serial.print(cycleR);
    Serial.print(F(" "));
    Serial.println(cycleL);

    preDF = df;
    preDR = dr;
    preDL = dl;
  }

  delay(100);
}

/**
 * Trigger an ultrasonic pulse and measure return echo distance in cm.
 * Operates in 3-pin mode (combined trigger and echo pin).
 */
long readUltrasonicDistance(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);

  pinMode(echoPin, INPUT);
  unsigned long duration = pulseIn(echoPin, HIGH, ULTRASONIC_TIMEOUT_US);
  return duration * 0.01723;
}

/**
 * Straight mode navigation: detect directional obstacles ahead.
 */
void straightMode(double df, double dr, double dl)
{
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);

  if (df <= DISTANCE_THRESHOLD && cycleF < CYCLE_LIMIT)
  {
    digitalWrite(VR_PIN, HIGH);
    digitalWrite(VL_PIN, HIGH);
  }
  else
  {
    if (dr <= DISTANCE_THRESHOLD && cycleR < CYCLE_LIMIT)
    {
      digitalWrite(VR_PIN, HIGH);
    }
    if (dl <= DISTANCE_THRESHOLD && cycleL < CYCLE_LIMIT)
    {
      digitalWrite(VL_PIN, HIGH);
    }
  }
}

/**
 * Down mode navigation: detect surface elevation (curb/step) or drop-offs (hole/stairs).
 */
void downMode(double df)
{
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);

  if (cycleF < CYCLE_LIMIT)
  {
    if (df <= level)
    {
      // Raised obstacle or step ahead -> right vibration warning
      digitalWrite(VR_PIN, HIGH);
    }
    else
    {
      // Drop-off or hole ahead -> left vibration warning
      digitalWrite(VL_PIN, HIGH);
    }
  }
}

/**
 * Calibrate the ground surface level by averaging 10 front sensor readings.
 */
void setLevel()
{
  double value = 0.0;
  int validSamples = 0;

  for (int i = 0; i < 10; i++)
  {
    double d = readUltrasonicDistance(SF_TRIG, SF_TRIG);
    if (d > 0)
    {
      value += d;
      validSamples++;
    }
    delay(100);
  }

  if (validSamples > 0)
  {
    level = value / validSamples;
  }
  else
  {
    level = 100.0; // Fallback baseline if calibration sensor is blocked
  }

  Serial.print(F("Calibrated Ground Level: "));
  Serial.println(level);
}

/**
 * Increment cycle counters if readings remain unchanged between consecutive loops.
 */
void updateCycleCount(double df, double dr, double dl)
{
  cycleF = (preDF == df) ? cycleF + 1 : 0;
  cycleR = (preDR == dr) ? cycleR + 1 : 0;
  cycleL = (preDL == dl) ? cycleL + 1 : 0;
}
