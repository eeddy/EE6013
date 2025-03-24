#include <DynamixelShield.h>

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
  #include <SoftwareSerial.h>
  SoftwareSerial soft_serial(7, 8); // DYNAMIXELShield UART RX/TX
  #define DEBUG_SERIAL soft_serial
#elif defined(ARDUINO_SAM_DUE) || defined(ARDUINO_SAM_ZERO)
  #define DEBUG_SERIAL SerialUSB    
#else
  #define DEBUG_SERIAL Serial
#endif

const float DXL_PROTOCOL_VERSION = 2.0;
const int BENTO_NUM = 5;

DynamixelShield dxl;

//This namespace is required to use Control table item names
using namespace ControlTableItem;

// Define global array of structs to hold the parameters for each digit
struct motorParam {
  int enabled;             // 0 = disabled (not connected), 1 = enabled (connected)
  int CW_limit;            // the CW position limit for the servo (varies from 0-4095)
  int CCW_limit;           // the CCW position limit for the servo (varies from 0-4095)
  int min_vel;             // the minimum velocity (varies from 0-1023)
  int max_vel;             // the maximum velocity (varies from 0-1023)
  int state;               // 0 = stopped, 1 = moving CW, 2 = moving CCW
  int pos;                 // the current target position
  int pos_;                // the present position (feedback from servo)
  int vel;                 // the current target velocity
  int vel_;                // the present velocity (feedback from servo)
} motor[BENTO_NUM+1];

// Assign joystick axis and buttons to pins on the arduino
// Also connect the joystick VCC to Arduino 5V, and joystick GND to Arduino GND.

const int VERT = A0; // analog
const int HORIZ = A1; // analog
const int SEL =  5; // digital
int prev_state = 1;
int servoID = 1;
int chA_deadband = 20;  
int chA_neutral = 512;
int chB_deadband = 20; 
int chB_neutral = 512;

int state = 1;

// This sketch outputs serial data at 115200 baud (open Serial Monitor to view).

void setup()
{
  // make the SEL line an input
  pinMode(SEL,INPUT_PULLUP);
  
  // Define shoulder rotate paramaters (X-series)
  motor[1].enabled = 0;       // disabled by default unless detected via pin command
  motor[1].CW_limit = 1500;       // the CW position limit for the servo
  motor[1].CCW_limit = 3073;   // the CCW position limit for the servo
  motor[1].min_vel = 1;           // the minimum velocity
  motor[1].max_vel = 50;         // the maximum velocity
  motor[1].pos = 2047;            // the current target position
  motor[1].vel = 10;           // the current target velocity

  // Define elbow extension paramaters (X-series)
  motor[2].enabled = 0;       // disabled by default unless detected via pin command
  motor[2].CW_limit = 1730;       // the CW position limit for the servo
  motor[2].CCW_limit = 2700;   // the CCW position limit for the servo
  motor[2].min_vel = 1;           // the minimum velocity
  motor[2].max_vel = 50;         // the maximum velocity
  motor[2].pos = 2047;            // the current target position
  motor[2].vel = 10;           // the current target velocity

  // Define wrist rotation paramaters (X-series)
  motor[3].enabled = 0;       // disabled by default unless detected via pin command
  motor[3].CW_limit = 1028;       // the CW position limit for the servo
  motor[3].CCW_limit = 3073;   // the CCW position limit for the servo
  motor[3].min_vel = 1;           // the minimum velocity
  motor[3].max_vel = 60;         // the maximum velocity
  motor[3].pos = 2047;            // the current target position
  motor[3].vel = 10;           // the current target velocity

  // Define wrist flexion paramaters (X-series)
  motor[4].enabled = 0;       // disabled by default unless detected via pin command
  motor[4].CW_limit = 848;       // the CW position limit for the servo
  motor[4].CCW_limit = 3248;   // the CCW position limit for the servo
  motor[4].min_vel = 1;           // the minimum velocity
  motor[4].max_vel = 60;         // the maximum velocity
  motor[4].pos = 2047;            // the current target position
  motor[4].vel = 10;           // the current target velocity

  // Define hand close/open paramaters (X-series)
  motor[5].enabled = 0;       // disabled by default unless detected via pin command
  motor[5].CW_limit = 1650;       // the CW position limit for the servo
  motor[5].CCW_limit = 2658;   // the CCW position limit for the servo
  motor[5].min_vel = 1;           // the minimum velocity
  motor[5].max_vel = 60;         // the maximum velocity
  motor[5].pos = 2047;            // the current target position
  motor[5].vel = 10;           // the current target velocity

 
  // For Uno, Nano, Mini, and Mega, use UART port of DYNAMIXEL Shield to debug.
  DEBUG_SERIAL.begin(115200);

  // Set Port baudrate to 1000000 bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(1000000);
  
  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);


  // Get DYNAMIXEL information and initialize the connected servos
  delay(100);  // Add delay so there is time for the bus to initialize before starting to ping servos
  for(int i=1; i<=BENTO_NUM; i++)
  {
    motor[i].enabled = dxl.ping(i);
    dxl.torqueOff(i);   // Turn off torque when configuring items in EEPROM area
    dxl.setOperatingMode(i, OP_POSITION);  // Set operating mode to position control mode
    dxl.torqueOn(i);    // Turn the torque back on
    // Read the latest position and velocity values from the servo
    motor[i].pos_ = dxl.getPresentPosition(i);
    motor[i].vel_ = dxl.getPresentVelocity(i);
    StopVelocity(i);    // Set the initial positions equal to the current positions to ensure each servo is stopped
  }
  dxl.ledOn(servoID);   // Set the LED of the initially selected servo in the sequential switching list
}

void loop() 
{
  StopVelocity(1);
  StopVelocity(5);
  delay(200);
  // read values from the external potentiometer
  int potValue = analogRead(VERT); // will be 0-1023
  int select = digitalRead(SEL); // will be HIGH (1) if not pressed, and LOW (0) if pressed

  servoID = 5;
  motor[servoID].pos_ = dxl.getPresentPosition(servoID);
  motor[servoID].vel_ = dxl.getPresentVelocity(servoID);
  motor[servoID].pos = motor[servoID].CCW_limit;
  motor[servoID].vel = map(potValue, chA_neutral - chA_deadband, 0, motor[servoID].min_vel, motor[servoID].max_vel);
  dxl.setGoalPosition(servoID, motor[servoID].pos);
  dxl.writeControlTableItem(PROFILE_VELOCITY, servoID, motor[servoID].vel);
  motor[servoID].state = 1;  
  DEBUG_SERIAL.print(servoID);
  DEBUG_SERIAL.print(", Present Position: ");
  DEBUG_SERIAL.print(motor[servoID].pos_);
  DEBUG_SERIAL.print(", Target Velocity: ");
  DEBUG_SERIAL.print(motor[servoID].vel);
  DEBUG_SERIAL.print(", Present Velocity: ");
  DEBUG_SERIAL.println(motor[servoID].vel_);
  delay(200);
  StopVelocity(servoID);

  servoID = 5;
  motor[servoID].pos_ = dxl.getPresentPosition(servoID);
  motor[servoID].vel_ = dxl.getPresentVelocity(servoID);
  motor[servoID].pos = motor[servoID].CCW_limit;
  motor[servoID].vel = map(potValue, chA_neutral - chA_deadband, 0, motor[servoID].min_vel, motor[servoID].max_vel);
  dxl.setGoalPosition(servoID, motor[servoID].pos);
  dxl.writeControlTableItem(PROFILE_VELOCITY, servoID, motor[servoID].vel);
  motor[servoID].state = 2;  
  DEBUG_SERIAL.print(servoID);
  DEBUG_SERIAL.print(", Present Position: ");
  DEBUG_SERIAL.print(motor[servoID].pos_);
  DEBUG_SERIAL.print(", Target Velocity: ");
  DEBUG_SERIAL.print(motor[servoID].vel);
  DEBUG_SERIAL.print(", Present Velocity: ");
  DEBUG_SERIAL.println(motor[servoID].vel_);
  delay(300);
  StopVelocity(servoID);

}  

// Stop the active servo by setting its target position equal to its current position
void StopVelocity(int j)
{
  // Offset by the value of the velocity to avoid jump back from feedback delay
  if (motor[j].pos == motor[j].CW_limit)
  {
    motor[j].pos = motor[j].pos_ + motor[j].vel_;
  }
  else if (motor[j].pos == motor[j].CCW_limit)
  {
    motor[j].pos = motor[j].pos_ + motor[j].vel_;
  }
  else
  {
    motor[j].pos = motor[j].pos_; // Set target position equal to the present position
  }
  motor[j].vel = motor[j].max_vel;
  dxl.setGoalPosition(j, motor[j].pos);
  dxl.writeControlTableItem(PROFILE_VELOCITY, j, motor[j].vel);
}
