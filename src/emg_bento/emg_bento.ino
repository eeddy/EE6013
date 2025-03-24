/*******************************************************************************
* Copyright 2016 ROBOTIS CO., LTD.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

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

const uint8_t DXL_ID = 4;
const float DXL_PROTOCOL_VERSION = 2.0;

struct motors {
  int ID;
  float min_pos;        
  float max_pos;                
  float actual_pos;                     
  float desired_vel;            
  float actual_vel;           
} motor[2];

DynamixelShield dxl;

using namespace ControlTableItem;

bool parse_command(String input, float &num1, float &num2) {
    char buf[20];
    input.toCharArray(buf, sizeof(buf));
    
    char *token = strtok(buf, ";"); 
    if (token != NULL) {
        num1 = atof(token); 
        token = strtok(NULL, ";");
        if (token != NULL) {
            num2 = atof(token);
            return true; 
        }
    }
    return false;
}

float clip(float value, float min, float max) {
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

void setup() {
  motor[0].ID = 4;
  motor[0].min_pos = 900;        
  motor[0].max_pos = 3100;                
  motor[0].actual_pos = 0;                     
  motor[0].desired_vel = 0;            
  motor[0].actual_vel = 0; 

  motor[1].ID = 5;
  motor[1].min_pos = 1720;        
  motor[1].max_pos = 2750;                
  motor[1].actual_pos = 0;                     
  motor[1].desired_vel = 0;            
  motor[1].actual_vel = 0; 

  DEBUG_SERIAL.begin(115200);
  dxl.begin(1000000);
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  for(int i=0; i<2; i++){
    dxl.ping(motor[i].ID);
    dxl.torqueOff(motor[i].ID);
    dxl.setOperatingMode(motor[i].ID, OP_VELOCITY);
    dxl.torqueOn(motor[i].ID);
  }
}

String input;
float vel_0 = 0;
float vel_1 = 0;

void loop() {
  // put your main code here, to run repeatedly:

    if (Serial.available() > 0) {
      input = Serial.readStringUntil('\n'); // Read input until newline
    }
      
    if (parse_command(input, vel_0, vel_1)) {
        motor[0].desired_vel = clip(vel_0, -10, 10);
        motor[1].desired_vel = clip(vel_1, -10, 10);
    } else {
        Serial.println("Invalid input");
    }

    for(int i=0; i<2; i++){
      dxl.setGoalVelocity(motor[i].ID, motor[i].desired_vel, UNIT_PERCENT);
      DEBUG_SERIAL.print("Motor: ");
      DEBUG_SERIAL.print(motor[i].ID);
      DEBUG_SERIAL.print("; Des Velocity: ");
      DEBUG_SERIAL.print(motor[i].desired_vel);
      motor[i].actual_vel = dxl.getPresentVelocity(motor[i].ID);
      motor[i].actual_pos = dxl.getPresentPosition(motor[i].ID);
      DEBUG_SERIAL.print("%; Act Velocity: ");
      DEBUG_SERIAL.print(motor[i].actual_vel);
      DEBUG_SERIAL.print("%; Act Position: ");
      DEBUG_SERIAL.print(motor[i].actual_pos);
      DEBUG_SERIAL.print("; ");

      if (motor[i].actual_pos >= motor[i].max_pos || motor[i].actual_pos <= motor[i].min_pos){
        dxl.setGoalVelocity(motor[i].ID, 0, UNIT_PERCENT);
        }
    }
    DEBUG_SERIAL.println();
}
