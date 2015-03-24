/*
 * serial_command_handler.cpp
 *
 *  Created on: Mar 9, 2015
 *      Author: travis
 */
#include "serial_command_handler.h"

long baud = 115200;
bool commandMode = false;
bool changeMade = false;
long flashInterval = 500;
long lastUpdateInterval = 0;
bool rgbOn;

serial_command_handler::serial_command_handler(){
	Serial.begin(baud);
}

void serial_command_handler::checkPort(){
	if(Serial.available() != 0){
		String data = Serial.readString();
		if(data.equals("$$$")){
			Serial.println("CMD");
			commandMode = true;
			RGB.control(true);
			RGB.color(128, 0, 128);
			RGB.brightness(255, true);
			rgbOn = true;
			lastUpdateInterval = millis();
			while(commandMode){
				if(millis() > lastUpdateInterval + flashInterval){
					if(rgbOn){
						RGB.brightness(0, true);
						rgbOn = false;
						lastUpdateInterval = millis();
					}else{
						RGB.brightness(255, true);
						rgbOn = true;
						lastUpdateInterval = millis();
					}

				}
				if(Serial.available()){
					String rData = Serial.readString();
					if(rData.startsWith("setKey=")){
						byte keyData[16];
						rData.getBytes(keyData, 17, 7);
						char bufKey[32];
						sprintf(bufKey, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", keyData[0],keyData[1],keyData[2],keyData[3],keyData[4],keyData[5],keyData[6],keyData[7],keyData[8],keyData[9],keyData[10],keyData[11],keyData[12],keyData[13],keyData[14],keyData[15]);
						Serial.println(bufKey);
						for(int i = 0; i < 16; i++){
							EEPROM.write(i, keyData[i]);
						}
						changeMade = true;
					}
					if(rData.startsWith("getKey")){
						char bufKey[32];
						sprintf(bufKey, "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x", EEPROM.read(0),EEPROM.read(1),EEPROM.read(2),EEPROM.read(3),EEPROM.read(4),EEPROM.read(5),EEPROM.read(6),EEPROM.read(7),EEPROM.read(8),EEPROM.read(9),EEPROM.read(10),EEPROM.read(11),EEPROM.read(12),EEPROM.read(13),EEPROM.read(14),EEPROM.read(15));
						Serial.print("Key: ");
						Serial.print(bufKey);
					}
					if(rData.startsWith("eraseKey")){
						for(int i = 0; i < 16; i++){
							EEPROM.write(i,255);
						}
						Serial.println("Key Erased");
						changeMade = true;
					}
					if(rData.startsWith("setID=")){
						int stringLength = rData.length();
						//determine length of id
						int idLen = stringLength - 6;
						//make sure we got a proper ID
						if(idLen > 6 || idLen == 0){
							Serial.println("ID to long or empty");
						}else{
							//Clear device id string
							String deviceIDString = rData.substring(6, rData.length());

							//Create byte array to hold ID for data storing
							char * idArray = new char[deviceIDString.length()];

							//Populate array with bytes from new id string
							strcpy(idArray, deviceIDString.c_str());
							Serial.print("deviceIDString.getBytes: ");
							Serial.println((const char*)idArray);

							//Store new ID to memory
							for(int i = 0; i < 6; i++){
								EEPROM.write(i+16,idArray[i]);
							}
							//Set unused Device ID memory locations to 255 if they are blank
							int blankPos = 6 - idLen;
							for(int i = 0; i < blankPos; i++){
								EEPROM.write(i+16+idLen, 255);
							}
							Serial.print("ID Set: ");
							Serial.println(deviceIDString);
							changeMade = true;
						}
					}
					if(rData.startsWith("eraseID")){
						for(int i = 0; i < 6; i++){
							EEPROM.write(i+16, 255);
						}
						Serial.println("ID Erased");
						changeMade = true;
					}
					if(rData.startsWith("getID")){
						char storedIDArray[6];
						for(int i = 0; i < 6; i++){
							unsigned char uC = EEPROM.read(i+16);
							if(uC != 255){
								storedIDArray[i] = (char)uC;
							}
						}
						String storedIDString(storedIDArray);
						Serial.print("ID: ");
						Serial.println(storedIDArray);
					}
					if(rData.startsWith("EXIT")){
						Serial.println("Exiting command Mode");
						delay(50);
						RGB.control(false);
						if(changeMade){
							Serial.println("Rebooting");
							delay(200);
							System.reset();
						}
						break;

					}
				}
			}
		}else{
			//Trash data and return
			Serial.println("Not in Command Mode, send $$$ to enter command mode");
			return;
		}

	}
}



