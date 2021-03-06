//Experimental file for flow sensor codes
//Datasheet: https://www.mouser.ca/datasheet/2/187/honeywell-sensing-zephyr-digital-airflow-10-slpm-3-740283.pdf
#include <Wire.h>
//TWI pins location is predetermined by library
//SCL = A5;
//SDA = A4; 
const int address = 0x49; //default I2C address of flow sensor
int flowValue;
int flowOutput;
int fullScaleFlow = 300; //takes the value of 10, 15, 20, 50, 100, 200, or 300. Must be specified to interpret the data
char serialNo[4];
int count = -1;

void setup() {
  // put your setup code here, to run once:
  Wire.begin(); //join I2C bus as master
  Serial.begin(9600);
  delay(50); //allow flow sensor to perform warm up
  for (int i = 0; i<2; i++){
    Wire.requestFrom(address, 2); //flow sensor responds with 2 bytes
    Serial.print("Flow Sensor Serial Number:  ");
    while (Wire.available()){
      char c = Wire.read(); //Read one byte a time
      strcat(serialNo,c);
      Serial.println(serialNo); //Read one byte a time
    } 
    delay(15); //command response time
  } //First 2 read sequences return serial number
}

void loop() {
  // put your main code here, to run repeatedly:
  Wire.requestFrom(address,2); //flow sensor responds with 2 bytes
  while(Wire.available()){ 
    char c = Wire.read();
    strcat(flowOutput, c);
    if (bitRead(flowOutput,15)==0 && bitRead(flowOutput,16)==0){
      Serial.print("Flow Ouput:  ");
      Serial.println(flowOutput);
      flowValue = fullScaleFlow*((flowOutput/16384)-0.1)/0.8; //equation given in the datasheet
      Serial.print("Flow Applied:  ");
      Serial.println(flowValue);
    } //The first ﻿two bits of each valid flow reading will be 00, bitRead() reads from rightmost bit
    else {
      Serial.print("Error reading flow value!");
    }
  }
}
