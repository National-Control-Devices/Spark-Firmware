/*
 * i2c_handler.cpp
 *
 *  Created on: Mar 10, 2015
 *      Author: travis
 */
#include "i2c_handler.h"

i2c_handler::i2c_handler(){

}

unsigned char * i2c_handler::read(unsigned char* buffer, size_t bufferSize){
	//Create unsigned char array to hold return bytes
	unsigned char rData[(int)bufferSize];
	//Start up the I2C port
	Wire.begin();
	//Open connection to chip
	Wire.beginTransmission(rData[0]);
	//Write the begin read memory address to the chip
	Wire.write(rData[1]);
	//Stop writing and get status indicating if the command worked
	byte status = Wire.endTransmission();
	//Read the return bytes from the chip
	Wire.requestFrom(buffer[0], buffer[2]);
	for(int i = 0; i < (int)bufferSize; i++){
		rData[i] = Wire.read();
	}
	//Return byte array of data read from chip
	return rData;
}

unsigned char i2c_handler::write(unsigned char* buffer, size_t bufferSize){
	//Open I2C port
	Wire.begin();
	//Open connection to chip
	Wire.beginTransmission(buffer[0]);
	//Set write address of chip
	Wire.write(buffer[1]);
	//Write data to address(s)
	for(int i = 2; i < (int)bufferSize; i++){
		Wire.write(buffer[i]);
	}
	//Check status of write commands
	unsigned char status = Wire.endTransmission();
	//Return status of write commands
	return status;
}

bool i2c_handler::checkIsRead(unsigned char* buffer, size_t bufferSize){
	if(buffer[0] & 0){
		return true;
	}else{
		return false;
	}
}

unsigned char i2c_handler::setAddress(unsigned char addr){
	unsigned char rAddr = addr >> 1;
	return rAddr;
}

