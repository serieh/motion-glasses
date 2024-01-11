// importing the required library 
#include <Wire.h>
#include <MPU6050.h>

// Define threshold for clarity in(cm)
#define DISTANCE_THRESHOLD 100  

//Storing the previous values for the distance 
double preDF = 0;
double preDR = 0;
double preDL = 0;

//the ultra-sonic sensor signal pins
const int SL_TRIG = 5;
const int SR_TRIG = 6;
const int SF_TRIG = 9;

//the vibrators pins
const int VL_PIN = 2;
const int VR_PIN = 3;

// the slide switches signal pins.
const int offPin = 7;
const int strPin = 4;

//the surface level distance for the user.
double level;

//create object of the MPU6050 library .
MPU6050 mpu;

//initialize the program.
void setup()
{
//start the communication with the port  (9600)
  Serial.begin(9600);

//defining the pinMode for the vibrator pins.
  pinMode(VL_PIN, OUTPUT);
  pinMode(VR_PIN, OUTPUT);
//defining the pinMode for the slide switches signal pins.
  pinMode(offPin, INPUT);
  pinMode(strPin, INPUT);

  //begin communications with the MPU6050 chip
  Wire.begin();
  mpu.begin();

//calculating the offset for the MPU6050 chip.
  mpu.calcOffsets(true, true);

//making a one second delay
  delay(1000);

//calling the function set level to assign the surface distance value for the level variable.
  setLevel();
}

//continuously executing the following code block
void loop()
{
//extracting the raw accelerometer vector.
  Vector normAccel = mpu.readNormalizeAccel();
  // Calculate pitch angle using accelerometer data
  float pitch = -1 * asin(normAccel.XAxis) * 180 / PI;
//Printing the pitch angle
  Serial.print("Pitch angle ");
  Serial.print(pitch);
//collecting the ultra-sonic distance for each sensor in (cm).
  double df = readUltrasonicDistance(SF_TRIG, SF_TRIG);
  double dr = readUltrasonicDistance(SR_TRIG, SR_TRIG);
  double dl = readUltrasonicDistance(SL_TRIG, SL_TRIG);
//Printing the Ultrasonic Distances for all sensors
  Serial.print("The Ultrasonic Distances for all sensors ");
  Serial.print(df);
  Serial.print(" ");
  Serial.print(dr);
  Serial.print(" ");
  Serial.println(dl);

//checking if there is an error or not in the ultra-sonic sensors
  if ((df == 0 || dr == 0 || dl == 0))
  {
//printing an error message.
    Serial.println("Error: Invalid distance readings");

  }else{
//checking if the previous value for the front ultra-sonic sensor does not equal the current value and the rest slide switch is off.
    if(preDF != df && digitalRead(offPin) == 0){
// checking if the pitch is above (30) degree or the straight-mode slide switch is on.
      if (pitch > 30 || digitalRead(strPin) == 1)
      {
//calling the straight-mode function.
        straightMode(df, dr, dl);
      }
      else
      {
//calling the down-mode function.
        downMode(df);
      }
    }else{
//incase the rule did not apply set all vibrators off.
      digitalWrite(VR_PIN, LOW);
      digitalWrite(VL_PIN, LOW);
    }
//assigning the current values as the previous values.
    preDF = df;
    preDR = dr;
    preDL = dl;
  }
//delay (100ms)
  delay(100);
}

// the read ultra sonic function
long readUltrasonicDistance(int triggerPin, int echoPin)
{
  pinMode(triggerPin, OUTPUT); // Clear the trigger
  digitalWrite(triggerPin, LOW);
  delayMicroseconds(2);

  // Sets the trigger pin to HIGH state for 10 microseconds
  digitalWrite(triggerPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(triggerPin, LOW);
  pinMode(echoPin, INPUT);

  // Reads the echo pin, and returns the distance in cm
  return pulseIn(echoPin, HIGH) * 0.01723;
}

//the straight mode functions.
void straightMode(double df, double dr, double dl)
{
// set all vibrators off 
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);

//checking if the front distance is lower or equal distance to the DISTANCE THRESHOLD.
  if (df <= DISTANCE_THRESHOLD)
  {
    digitalWrite(VR_PIN, HIGH);
    digitalWrite(VL_PIN, HIGH);
  }
  else
  {
//checking if the right distance is lower or equal distance to the DISTANCE THRESHOLD.
    if (dr <= DISTANCE_THRESHOLD)
    {
      digitalWrite(VR_PIN, HIGH);
    }

//checking if the left distance is lower or equal distance to the DISTANCE THRESHOLD.
    if (dl <= DISTANCE_THRESHOLD)
    {
      digitalWrite(VL_PIN, HIGH);
    }
  }
}

//the down-mode function.
void downMode(double df)
{
// set all vibrators off 
  digitalWrite(VR_PIN, LOW);
  digitalWrite(VL_PIN, LOW);

//checking if the front distance is lower or equal to the surface level.
  if (df <= level)
  {
    digitalWrite(VR_PIN, HIGH);
  }
//if it is above the surface level.
  else
  {
    digitalWrite(VL_PIN, HIGH);
  }
}

//calculating and assigning the correct surface level value for the user.
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
