// If not properly set up, go to Sketch > Include Library > Manage Library. Search for and install required libraries:
#include <Wire.h> //library to control I2C bus from the flow sensor
#include <Servo.h> //library to control the servo
#include <SoftwareSerial.h> //library for bluetooth communication with HC-05 BT-module

////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// BEGIN INITIALIZING GLOBALS ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

#define SIZE_OF 4  // Size of the finger array.  Used in loops that operate on all the fingers.
// constants for automated flex sensor threshold logic and automated emergency shutoff.
#define SAFETY_THRESHOLD_HIGH 1000   // All values compared to input from trueFlex. TODO: Verify proper value via testing
#define SAFETY_THRESHOLD_LOW -100    // TODO: Verify proper value via testing
#define MAINTENANCE_THRESHOLD 500    // Used in the main "void loop()" function.
#define TOLERANCE 100      //The range during which pressure will be maintained
#define ACCEPTABLE_CALIBRATION_RANGE 300  // Calibration is deemed a failure if we
                                               // do not have a minimum range between high
                                               // and low sensor readings.  Currently
                                               // causes emergency shutoff.  In the future
                                               // could be used to restart calibration.

Servo servoControl;  // create servo object to control a servo

// Constants for the Flex Sensor and Potentiometer Readings.
// This is detailing, the pin numbers that are used for input or output
SoftwareSerial Bluetooth(12, 13); // pin 12, pin 11 for BT communication.
                                  // RX, TX  (NOTE: RX is wired into HC-05 TXD,
                                  //                and TX is wired into HC-05 RXD)
                                  // RX = Recieve, TX = Transmission
const int flexInput = A1; // Analog input 1, for flex sensor input.
const int fingerValve1 = 3;  // Regular finger valve
const int fingerValve2 = 4;  // Regular finger valve
const int fingerValve3 = 5;  // Regular finger valve
const int fingerValve4 = 6;  // Regular finger valve
const int fingerValves [SIZE_OF] = {fingerValve1, fingerValve2, fingerValve3, fingerValve4}; // Array to organize finger valve pin definitions.
const int emergencyValve = 7; // Emergency release valve
const int MOTOR_FORWARD = 8; // forward motor control
const int emergencyButton = 9; // Emergency release button.

// constants defining different therapy modes.  When the value of therapyMode is set
// to the value of one of these constants, the associated therapyMode will be executed by
// the switch statement.
const int idleMode = 0;
const int calibrationMode = 1;
const int inflateMode = 2;


int therapyMode; // Change this value to change the mode that the program runs in.
                 // Can be changed at runtime, but a buffering state should be used
                 // when switching between modes.

// These constants are used for calibrating the flex sensor at the start of the trials.
// These will be eventually replaced when we can
// hold a consistant calibration between trails (aka signal conditioning).
const int INIT_FLEX_LOW = 1023;
const int INIT_FLEX_HIGH = 0;

int flexSensorHigh = INIT_FLEX_HIGH; // INITIALIZATION: Will be the HIGHER number after the calibration loop.
int flexSensorLow = INIT_FLEX_LOW; // INITIALIZATION: This number will be the lower of the 2 after
                                   // the calibration loop.

int flexValue; // The value read from the flex sensor.
int trueFlex; // Will be the remapped value read from the flex sensor.
int emergencyButtonInput; // Reads
int angleControl; //takes value from 0 to 180 (need to be remapped) to control the opening of the exhaust valve

////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////// END INITIALIZING GLOBALS ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// BEGIN SETUP FUNCTIONS ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

/*
 * Initialize all output/input pins.
 */
void initializePins(){
  for(int i=0; i<SIZE_OF; i++){
    pinMode(fingerValves[i], OUTPUT);  // Initialize all 4 finger valves.
  }
  pinMode(emergencyValve, OUTPUT);

  pinMode(MOTOR_FORWARD, OUTPUT); // Takes variable integer values to set the speed of the motor.

  pinMode(emergencyButton, INPUT); // Used to stop the motor, and release the pressure in the system.
  servoControl.attach(10); //Attach the Servo to Pin 10
}


////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////// END SETUP FUNCTIONS ////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// BEGIN SETUP /////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

/*
 * Initialize the setup.
 *
 * Runs automatically at the beginning of any program.  Use to initialize all
 * input and output pins.
 */
void setup() {
  // These commands are for the serial monitor, which can be found under tools tab up top.
  // This is useful for reporting of the potentiometer value as well as the resistance value
  // of the flex sensor (mapped between 0 and 1023)
  Serial.begin(9600);
  while(!Serial){ // TODO: Remove or modify if we switch to using BT exclusively.
    // wait for Serial port to connect.  Needed for native USB port only.
  }
  Bluetooth.begin(9600); // Initialize BT serial communication on pins 11 and 12
  while(!Bluetooth.isListening()){
    Serial.println("Waiting for Bluetooth...");
  }

  // Setting the Pinmodes of the arduino, these may be changed later base on the
  // board that we end up using in future versions
  initializePins();

  // OUTPUT: Tests the initialization of the pins.
  testingSetupOutput();

  // TEMP/PRESENTATION
  flexSensorHigh = 1000;
  flexSensorLow = 200;

  therapyMode = idleMode;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// END SETUP ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// BEGIN MAIN LOOP /////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  /////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////// BEGIN EMERGENCY SHUTOFF ////////////////////////////////////////////
  // Returns 0 or 1.                                                                        ///
  // 0 means 0V, 1 for 5V                                                                   ///
  // 0V == LOW, 5V == HIGH                                                                  ///
  //////////////// TEMP: An electrical solution will be hardwired in the future. //////////////
  emergencyButtonInput = digitalRead(emergencyButton);
  if(emergencyButtonInput == HIGH){
    emergencyShutoff("Emergency button pressed.");
  }
  //////////////////////// END EMERGENCY SHUTOFF ///////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////// BEGIN READING SYSTEM INPUT //////////////////////////////////////////
  // Reading of the flex sensor value that is attached to the finger balloon
  // The map function is used to increase our resolution from whatever the
  // calibrated range is to the maximum possible range (0-1023, based on 8 bit
  // resolution of arduino uno)
  flexValue=analogRead(flexInput);

  // The flex value remapped to take advantage of our full range of possible values.
  // NOTE: This can be less than 0 or higher than 1023 in some cases.  This is due to
  // the remapping and can be used to establish upper and lower bounds on flex if desired.
  trueFlex=map(flexValue, flexSensorLow, flexSensorHigh, 0, 1023);

  // Workaround, analogue synthesizer.  Simulate voltages between 0V and 5V
  // Control the motor speed.

  /////////////////////////////// END READING SYSTEM INPUT /////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // OUTPUT: Prints the flex values ect to Serial output. //////////////////////////////////////////

  loopTestingOutput();                                                                  ////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////

  // Turning the light on when the the value of the mapped flex sensor matches within +/- 50 of the
  // potentiometer value. The delay is set so the arduino checks every 300 ms and reports the value
  // to the serial monitor. All values here are subject to change based on requirements of system.

  ///////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////// BEGIN READ SERIAL INPUT ////////////////////////////////////////

  if(Bluetooth.available()){
    // Looking for values from 48 to 57 (ASCII '0' to ASCII '9')
    int tempTherapyMode = 1 * (Bluetooth.read() - '0');
    // TODO: Bug when reading Bluetooth Serial, seems to send 3 instances of the same character.  
    // Could be an issue with our code, but it worked for normal Serial so I doubt it.  
    // Probably an issue with the Android Serial Terminal app (maybe it is using a different baud rate?)
    // Tested against equivalent usage with BT and a serial console in Windows.  The Android Serial Terminal app is buggy, not our code.
    if(tempTherapyMode >= 0 && tempTherapyMode < 10){
      therapyMode = tempTherapyMode;
      loopReadingInput();
      delay(1000);
    }
  }

  /////////////////////////////// END READ SERIAL INPUT /////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////

  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////// BEGIN MODE-CHANGE STATEMENT /////////////////////////////////////////

  switch(therapyMode){
    // Serial output is written by the end effecting functions in this section.  This allows for the cleanest code and least chance of surprises.
    case idleMode:
      activateIdleMode();
      break;
    case calibrationMode:
      calibrateFlexSensor();
      break;
    case inflateMode :
      activateInflateMode();
      break;
    // More modes can be added here as desired.  Please add a funtion to the Therapy_Modes file
    // to perform the necessary actions as defined by your mode.
  }

  //////////////////////////// END MODE-CHANGE STATEMENT ////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// END MAIN LOOP ///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////




////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// FUTURE NOTES ////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
