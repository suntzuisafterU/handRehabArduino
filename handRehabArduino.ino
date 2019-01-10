// If not properly set up, go to Sketch > Include Library > Manage Library. Search for and install required libraries:
#include <Wire.h> //library to control I2C bus from the flow sensor
#include <Servo.h> //library to control the servo

////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// BEGIN INITIALIZING GLOBALS ///////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////

Servo servoControl;  // create servo object to control a servo

// Constants for the Flex Sensor and Potentiometer Readings. 
// This is detailing, the pin numbers that are used for input or output 
const int flexInput = A1; // Analog input 1, for flex sensor input.
const int fingerValve =  5;  // Regular finger valve
const int emergencyValve = 7; // Emergency release valve
const int emergencyButton = 8; // Emergency release button.
const int MOTOR_FORWARD = 9; // forward motor control

// constants for automated flex sensor threshold logic and automated emergency shutoff.
const int SAFETY_THRESHOLD_HIGH = 1000;   // All values compared to input from trueFlex.
const int SAFETY_THRESHOLD_LOW = -100;    //
const int MAINTENANCE_THRESHOLD = 500;    // Used in the main "void loop()" function.
const int TOLERANCE = 100;      //The range during which pressure will be maintained
const int ACCEPTABLE_CALIBRATION_RANGE = 300;  // Calibration is deemed a failure if we 
                                               // do not have a minimum range between high
                                               // and low sensor readings.  Currently 
                                               // causes emergency shutoff.  In the future
                                               // could be used to restart calibration.

// constants defining different therapy modes.  When the value of therapyMode is set
// to the value of one of these constants, the associated therapyMode will be executed by
// the switch statement.
const int idleMode = 0;
// SPECTRUM: Removed for presentation mode.
//const int calibrationMode = 1;
const int inflateMode = 2;


int therapyMode; // Change this value to change the mode that the program runs in.
                 // Can be changed at runtime, but a buffering state should be used
                 // when switching between modes.

// These constants are used for calibrating the flex sensor at the start of the trials. 
// These will be eventually replaced when we can
// hold a consistant calibration between trails (aka signal conditioning).
const int INIT_FLEX_LOW = 1023;
const int INIT_FLEX_HIGH = 0;
int flexSensorLow = INIT_FLEX_LOW; // INITIALIZATION: This number will be the lower of the 2 after 
                          // the calibration loop.
int flexSensorHigh = INIT_FLEX_HIGH; // INITIALIZATION: Will be the HIGHER number after the calibration loop.

int flexValue; // The value read from the flex sensor.
int trueFlex; // Will be the remapped value read from the flex sensor.
int emergencyButtonInput; // Reads 
int angleControl; //takes value from 0 to 180 (need to be remapped) to control the opening of the exhaust valve

int PWM = 0; // Controls motor driver. Value betweem 0-255. GETS MAPPED FROM MAP 0-1023!

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
  pinMode(fingerValve, OUTPUT);
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
  while(!Serial){
    // wait for Serial port to connect.  Needed for native USB port only.
  }

  // Setting the Pinmodes of the arduino, these may be changed later base on the 
  // board that we end up using in future versions 
  initializePins();

  // OUTPUT: Tests the initialization of the pins.
  // SPECTRUM: Commented out temporarily
  // testingSetupOutput();

  therapyMode = idleMode;
  // SPECTRUM: Temp hard coded calibration values
  flexSensorLow = 400;
  flexSensorHigh = 900;
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
    emergencyShutoff();
  }
  //////////////////////// END EMERGENCY SHUTOFF ///////////////////////////////////////////////
  //////////////////////////////////////////////////////////////////////////////////////////////

  //////////////////////////////////////////////////////////////////////////////////////////////
  //////////////////////// BEGIN READING SYSTEM INPUT //////////////////////////////////////////
  // Reading of the flex sensor value that is attached to the finger balloon
  // The map function is used to increase our resolution from whatever the 
  // calibrated range is to the maximum possible range (0-1023, based on 8 bit 
  // resolution of arduino uno)
  flexValue= analogRead(flexInput);

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
  // SPECTRUM: Commented out temporarily
  // loopTestingOutput();                                                                  ////////////
  //////////////////////////////////////////////////////////////////////////////////////////////////
  
  // Turning the light on when the the value of the mapped flex sensor matches within +/- 50 of the 
  // potentiometer value. The delay is set so the arduino checks every 300 ms and reports the value 
  // to the serial monitor. All values here are subject to change based on requirements of system. 

  ///////////////////////////////////////////////////////////////////////////////////////////////
  ////////////////////////////// BEGIN READ SERIAL INPUT ////////////////////////////////////////
  
  if(Serial.available()){
    int tempTherapyMode = 1 * (Serial.read() - '0');
    // TEMP/TESTING
    if(tempTherapyMode > 0 && tempTherapyMode < 10){
      therapyMode = tempTherapyMode;
    }
    Serial.print("Changed Therapy Mode:");
    Serial.println(therapyMode); 
  }
  
  /////////////////////////////// END READ SERIAL INPUT /////////////////////////////////////////
  ///////////////////////////////////////////////////////////////////////////////////////////////
  
  ///////////////////////////////////////////////////////////////////////////////////////////////
  ///////////////////////// BEGIN MODE-CHANGE STATEMENT /////////////////////////////////////////

  switch(therapyMode){
    case idleMode:
      activateIdleMode();
      idlePresentationOutput();
      break;
    // SPECTRUM: Commented out temporarily
//    case calibrationMode:
//      calibrateFlexSensor();
//      break;
    case inflateMode:
      activateInflateMode();
      inflatingPresentationOutput();
      break;
//    case default:
//      // SPECTRUM: Temporary print for presentation
//      Serial.println("WARNING: Unrecognized input");
//      break;
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
