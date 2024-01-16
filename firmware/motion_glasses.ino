//#include <Wire.h>

#define DISTANCE_THRESHOLD 100
#define CYCLE_LIMIT 46

double preDF = 0;
double preDR = 0;
double preDL = 0;

const int SL_TRIG = 5;
const int SR_TRIG = 6;
const int SF_TRIG = 9;

const int VL_PIN = 2;
const int VR_PIN = 3;

const int strPin = 4;
const int noCyPin = 13;

double level;

int downAngle = 30;
int tiltPin = 12;

int cycleF = 0;
int cycleR = 0;
int cycleL = 0;

void setup() {
  Serial.begin(9600);
  pinMode(VL_PIN, OUTPUT);
  pinMode(VR_PIN, OUTPUT);

  pinMode(strPin, INPUT);
  pinMode(noCyPin, INPUT);

  pinMode(tiltPin, INPUT);
  digitalWrite(tiltPin, HIGH);

  // Uncomment the following lines if you are using an MPU6050 sensor
  // Wire.begin();
  // mpu.begin();
  // mpu.calcOffsets(true, true);
  delay(1000);
  setLevel();
}

void loop() {
  // Uncomment the following lines if you are using an MPU6050 sensor
  // Vector normAccel = mpu.readNormalizeAccel();
  // float pitch = -1 * asin(normAccel.XAxis) * 180 / PI;
  float pitch; // Simulated pitch value for testing
  if (digitalRead(tiltPin) == 0) {
    pitch = 100;
  } else {
    pitch = 0;
  }

  Serial.print("Pitch angle ");
  Serial.println(pitch);

  double df = readUltrasonicDistance(SF_TRIG, SF_TRIG);
  double dr = readUltrasonicDistance(SR_TRIG, SR_TRIG);
  double dl = readUltrasonicDistance(SL_TRIG, SL_TRIG);

  Serial.print("The Ultrasonic Distances for all sensors ");
  Serial.print(df);
  Serial.print(" ");
  Serial.print(dr);
  Serial.print(" ");
  Serial.println(dl);

  if (df == 0 || dr == 0 || dl == 0) {
    Serial.println("Sleep Mode is Active");
  } else {
    if (pitch > downAngle || digitalRead(strPin) == 1) {
      straightMode(df, dr, dl);
    } else {
      downMode(df);
    }
  
    if (digitalRead(noCyPin) == 0) {
      updateCycleCount(df, dr, dl);
    } else {
      cycleF = 0;
      cycleR = 0;
      cycleL = 0;
    }

    Serial.print(cycleF);
    Serial.print(" ");
    Serial.print(cycleR);
    Serial.print(" ");
    Serial.println(cycleL);

    preDF = df;
    preDR = dr;
    preDL = dl;

    Serial.print(preDF);
    Serial.print(" ");
    Serial.print(preDR);
    Serial.print(" ");
    Serial.println(preDL);
  }

  delay(100);
}

long readUltrasonicDistance(int triggerPin, int echoPin) {
  pinMode(triggerPin, OUTPUT);
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);
  return pulseIn(echoPin, HIGH) * 0.01723;
}

void straightMode(double df, double dr, double dl) {
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);
  
  if (df <= DISTANCE_THRESHOLD && cycleF < CYCLE_LIMIT) {
    digitalWrite(VR_PIN, HIGH);
    digitalWrite(VL_PIN, HIGH);
  } else {
    if (dr <= DISTANCE_THRESHOLD && cycleR < CYCLE_LIMIT) {
      digitalWrite(VR_PIN, HIGH);
    }
    if (dl <= DISTANCE_THRESHOLD && cycleL < CYCLE_LIMIT) {
      digitalWrite(VL_PIN, HIGH);
    }
  }
}

void downMode(double df) {
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);
  
  if (cycleF < CYCLE_LIMIT) {
    if (df <= level) {
      digitalWrite(VR_PIN, HIGH);
    } else {
      digitalWrite(VL_PIN, HIGH);
    }
  }
}

void updateCycleCount(double df, double dr, double dl) {
  // Update cycle counts
  cycleF = (preDF == df) ? cycleF + 1 : 0;
  cycleR = (preDR == dr) ? cycleR + 1 : 0;
  cycleL = (preDL == dl) ? cycleL + 1 : 0;
}

void setLevel()
{
//collecting ten readings from the front ultra-sonic sensor.
  double value = 0;
  for (int i = 0; i < 10; i++)
  {
    value += readUltrasonicDistance(SF_TRIG, SF_TRIG);
    delay(100);
  }
//assigning to the level value the mean of the sum of all distances 
  level = value / 10;
}
