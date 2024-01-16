// importing the required library 
#include <Wire.h>
#include <MPU6050.h>

// Define threshold for clarity in(cm)
#define DISTANCE_THRESHOLD 100

//The number of cycls allowed
#define CYCLE_LIMIT 46

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
const int strPin = 4;
const int noCyPin = 13;

//the surface level distance for the user.
double level;

//The angle which under it switchs to downmode
int downAngle = 30;

//number of cycles for all sensors
int cycleF = 0;
int cycleR = 0;
int cycleL = 0;

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
  pinMode(noCyPin, INPUT);
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
//extracting the raw accelerometer victor.
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
//printing a Sleep mode message.
    Serial.println("Sleep Mode is Active");

  }else{
// checking if the pitch is above the downAngle degree or the straight-mode slide switch is on.
      if (pitch > downAngle || digitalRead(strPin) == 1)
      {
//calling the straight-mode function.
        straightMode(df, dr, dl);
      }
      else
      {
//calling the down-mode fumction.
        downMode(df);
      }

    
//check if no Cycle Mode is on or off to set the cycles values
    if(digitalRead(noCyPin) == 0){
    updateCycleCount(df, dr, dl);
    }else{
      cycleF = 0;
      cycleR = 0;
      cycleL = 0;
    }
//prints the cycles
    Serial.print(cycleF);
    Serial.print(" ");
    Serial.print(cycleR);
    Serial.print(" ");
    Serial.println(cycleL);
    
//assigning the current values as the previous values.
    preDF = df;
    preDR = dr;
    preDL = dl;

//prints the previous values.
    Serial.print(preDF);
    Serial.print(" ");
    Serial.print(preDR);
    Serial.print(" ");
    Serial.println(preDL);
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


//checking if the front distance is lower or equal distance to the DISTANCE THRESHOLD and if the front cycle is below the cycle LIMIT
  if (df <= DISTANCE_THRESHOLD && cycleF < CYCLE_LIMIT)
  {
    digitalWrite(VR_PIN, HIGH);
    digitalWrite(VL_PIN, HIGH);
  }
  else
  {
//checking if the right distance is lower or equal distance to the DISTANCE THRESHOLD and if the right cycle is below the cycle LIMIT

    if (dr <= DISTANCE_THRESHOLD && cycleR < CYCLE_LIMIT)
    {
      digitalWrite(VR_PIN, HIGH);
    }


//checking if the left distance is lower or equal distance to the DISTANCE THRESHOLD and if the left cycle is below the cycle LIMIT
    if (dl <= DISTANCE_THRESHOLD && cycleL < CYCLE_LIMIT)
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

//check if the front cycle is below the cycle LIMIT or not
  if (cycleF < CYCLE_LIMIT) {
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

void updateCycleCount(double df, double dr, double dl) {
  // Update cycle counts
  cycleF = (preDF == df) ? cycleF + 1 : 0;
  cycleR = (preDR == dr) ? cycleR + 1 : 0;
  cycleL = (preDL == dl) ? cycleL + 1 : 0;
}
