/**
 ******************************************************************************
 * @file    application.cpp
 * @authors  Satish Nair, Zachary Crockett and Mohit Bhoite
 * @version V1.0.0
 * @date    05-November-2013
 * @brief   Tinker application
 ******************************************************************************
  Copyright (c) 2013 Spark Labs, Inc.  All rights reserved.

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this program; if not, see <http://www.gnu.org/licenses/>.
 ******************************************************************************
 */

/* Includes ------------------------------------------------------------------*/  
#include "application.h"
#include "aes_client.h"
#include "HttpClient.h"
#include "spark_protocol.h"
#include "tropicssl/rsa.h"
#include "tropicssl/aes.h"
#include "spark_utilities.h"

//Variables for send request buffer to hold requests to send to server
//#define NREQS 5
//#define REQSZ 344
//char requestArray[NREQS][REQSZ];
//int readReq  = 0;
//int writeReq = 0;

//Variables for buffer to hold commands sent from server to run
#define NRUNCOMMANDS 16
#define RUNCOMMND 16
unsigned char runCommandsArray[NRUNCOMMANDS][RUNCOMMND];
int numberOfRunCommands = 0;

//Variables for buffer to hold monitor commands
#define NMONITORCOMMANDS 10
#define MONITORCOMMAND 16
unsigned char monitorCommands[NMONITORCOMMANDS][MONITORCOMMAND];
int numberOfMonitorCommands = 0;
#define NMONITORCOMPAREVARIABLES 10
#define MONITORCOMPAREVARIABLE 4
unsigned char monitorCompareVariables[NMONITORCOMPAREVARIABLES][MONITORCOMPAREVARIABLE];
#define NMONITORCOMPAREOPERATORS 10
#define MONITORCOMPAREOPERATOR 2
unsigned char monitorCompareOperators[NMONITORCOMPAREOPERATORS][MONITORCOMPAREOPERATOR];

//Variables for buffer to hold report commands
#define NREPORTCOMMANDS 10
#define REPORTCOMMAND 16
#define REPORTLENGTH 4
unsigned char reportCommandsArray[NREPORTCOMMANDS][REPORTCOMMAND];
unsigned char reportBytes[NREPORTCOMMANDS][REPORTLENGTH];
int numOfBytesInReport[REPORTCOMMAND];
int numberOfReportCommands = 0;
int numberOfReportBytes = 0;
unsigned char commandFailed[4] = {0,0,0,0};

SYSTEM_MODE(SEMI_AUTOMATIC);

void sendHTTPRequest(char message[], String action);
void initSession();
void parseServerResponse(String message);
void checkIn();
void sendMessage(String message);
void processReportCommands();
void processRunCommands();

//Variables for GPIOs
int button1 = D0;
int button2 = D1;
int button3 = D2;
int button4 = D3;
const char* b1Message ="Button 1";
const char* b2Message ="Button 2";
const char* b3Message ="Button 3";
const char* b4Message ="Button 4";
bool button1Send = true;
bool button2Send = true;
bool button3Send = true;
bool button4Send = true;

//Variables for TCP communication
int port = 6000;
//TCPServer server = TCPServer(port);
//TCPClient client;

//Variables for UDP broadcasts
UDP udp;
unsigned long lastBroadcastTime = 0;
unsigned long broadcastInterval = 8000;
IPAddress broadcastIP;
int remotePort = 6000;
void doBroadcast();
char mac[18];
void getMac();

//Variables for HTTP
int idLength = 0;
String deviceIDString;

HttpClient http;
http_header_t headers[] = {
    //  { "Content-Type", "application/json" },
    //  { "Accept" , "application/json" },
    { "Accept" , "*/*"},
//    {"Host", "cloud.controleverything.com"},
    { NULL, NULL } // NOTE: Always terminate headers will NULL
};
http_request_t request;
http_response_t response;
String action;
long initStart = 0;
long initInterval = 10000;

//Variables for AES
aes_context aes;
unsigned char key[16];
bool init = true;
unsigned char origionaliv[16];
char v_energy_pos[174];
aes_client aesClient;

//Test send repeatedly variables
long lastSendTime = 0;
long interval = 3000;
int sends = 0;

//Checks for sending enabled
bool sendEnabled = false;
bool gotKey = false;
bool gotHost = false;
bool gotID = false;

//Check in interval
long checkInInterval = 0;
long lastCheckIn = 0;
String version = "v1";

//Json Buffer size;
const size_t jSONBufferSize = 0;

serial_command_handler sHandler;

/* This function is called once at start up ----------------------------------*/

void setup()
{
	Serial.begin(115200);
	Serial1.begin(9600);

	WiFi.setCredentials("Official-Office-One", "1234567890");

	delay(500);

	WiFi.connect();

	while(!WiFi.ready());

	Serial.println("WiFi Connected, neat huh");

	if(http.getDNSHost("cloud.controlanything.com")){
		gotHost = true;
	}

	pinMode(button1, INPUT_PULLUP);
	pinMode(button2, INPUT_PULLUP);
	pinMode(button3, INPUT_PULLUP);
	pinMode(button4, INPUT_PULLUP);

	getMac();

	//Setup udp broadcast IP(ex. 192.168.0.255)
	broadcastIP = WiFi.gatewayIP();
	broadcastIP[3] = 255;
	udp.begin(5000);

	//Get AES master key out of EEPROM and put it in our key variable
	for(int i = 0; i < 16; i++){
		key[i] = EEPROM.read(i);
		if(key[i] != 255){
			gotKey = true;
		}

	}
	if(gotKey){
		aesClient.setKey(key);
	}

	//Get Device ID out of Memory max length of Device ID is 6.
	for(int i = 0; i < 6; i++){
		if(EEPROM.read(i+16) != 255){
			unsigned char u = EEPROM.read(i+16);
			char b = (char)u;
			deviceIDString+=b;
			idLength++;
		}
	}
	if(idLength != 0){
		gotID = true;
		aesClient.setID(deviceIDString);
	}
}

/* This function loops forever --------------------------------------------*/
void loop()
{
	start:
	//Check USB serial port for commands.  Will block if put into command mode.
	sHandler.checkPort();

	//Do checks
	if(gotHost && gotKey && gotID){
		sendEnabled = true;
	}else{
		sendEnabled = false;
	}

	if(init && sendEnabled){
			initSession();
	}

	//Send UDP packet every x seconds
	if(millis() > lastBroadcastTime + broadcastInterval){
		doBroadcast();
		lastBroadcastTime = millis();
	}

	if(Serial1.available() != 0){
		long timeout = 1000;
		long startReadTime = millis();
		char s1B[108];
		memset(s1B, 0, 128);
		while(millis()<startReadTime+timeout){
			Serial1.readBytes(s1B, sizeof s1B);
		}
		String message(s1B);
		sendMessage(message);
	}



	//Check in with server on interval
	if(checkInInterval != 0 && millis() > lastCheckIn + checkInInterval){
		checkIn();
	}

//	if(readReq != writeReq && init==false){
//		sendHTTPRequest(requestArray[readReq], action);
//		if(init == true){
//			goto start;
//		}
//		readReq++;
//		if(readReq==NREQS){
//			readReq=0;
//		}
//	}

	if(numberOfRunCommands != 0){
		processRunCommands();
	}

}

void checkIn(){
	processReportCommands();

	StaticJsonBuffer<500> jsonBuffer;
	JsonArray& root = jsonBuffer.createArray();
	for(int i = 0; i < numberOfReportCommands; i++){
		int reportPack[numOfBytesInReport[i]];
		for(int j = 0; j < numOfBytesInReport[i]; j++){
			reportPack[j] = reportBytes[i][j];
		}
		JsonArray& nestedReport = jsonBuffer.createArray();
		for(int j = 0; j < numOfBytesInReport[i]; j++){
//			Serial.print("adding int to json: ");
//			Serial.println(reportPack[j], 10);
			nestedReport.add(reportPack[j]);
		}
		root.add(nestedReport);
	}
	char buffer[256];
	root.printTo(buffer, sizeof buffer);
	String message(buffer);
	if(sendEnabled){
		char output[344];
		aesClient.cbcEncrypt(message, output);
		sendHTTPRequest(output, "connect");
	}
	return;
}

void sendMessage(String message){
	//Dont send empty messages.
	if(message.length() == 0){
		return;
	}

	Serial.print("Sending Message: ");
	Serial.println(message);

	if(sendEnabled){
		char output[344];
		aesClient.cbcEncrypt(message, output);
//		strcpy(requestArray[writeReq++], output);
	}
	return;

}

void initSession(){
	init = false;
	Serial.println("Initializing session");

	char base64String[172];
	aesClient.ecbEncrypt(base64String);

	sendHTTPRequest(base64String, "init");

//	//setup HTTP Request header
//	request.hostname = "cloud.controlanything.com";
//	request.port = 420;
//	request.path = "/device/"+deviceIDString+"/init/"+version+"?data="+base64String;
//	//	request.body = " ";
//
//	//Send request
//	Serial.println("Init sending get");
//	delay(100);
//	long start = millis();
//	http.get(request, response, headers);
//	int rStatus = response.status;
//
//	if(rStatus != 200){
//		Serial.print("Response Status on init: ");
//		Serial.println(rStatus);
//		init = true;
//		return;
//	}
//
//	Serial.print("Length of response from server: ");
//	Serial.println(response.responseLength);
//
//	Serial.println("about to decrypt packet");
//	delay(100);
//	String serverInitResponse = aesClient.cbcDecrypt(response.body, response.responseLength);
//	Serial.println("packet decrypted");
//	delay(100);
//
//	Serial.print("Decrypted: ");
//	Serial.println(serverInitResponse);
//	if(serverInitResponse.equals("Fail")){
//		init = true;
//		return;
//	}else{
//		Serial.print("Check in Interval");
//		Serial.println(checkInInterval);
//		parseServerResponse(serverInitResponse);
//		lastCheckIn = millis();
//		return;
//	}

}

void genericI2C(byte command[], size_t size){
	delay(100);
	Serial.println("Sending I2C command");
	delay(100);
	Wire.begin();

	//I2C get
	if(command[0] == 0){
		Serial.println("Get command");
		//read command
		//second byte is I2C bus device address
		//third byte is memory location to read from
		//fourth byte is amount of expected return data

		Wire.beginTransmission(command[1]);
		Wire.write(command[2]);
		byte status = Wire.endTransmission();
		if(status != 0){
			//Error
			return;
		}
		int length = Wire.requestFrom(command[1], command[3]);
		if(length != command[3]){
			//Did not get expected number of return bytes.  Something went wrong
			return;
		}
		byte returnData[length];
		for(int i = 0; i < length; i++){
			returnData[i] = Wire.read();
		}


	}
	//I2C set
	if(command[0] == 1){
		Serial.println("Set command");
		//write command
		//second byte is I2C device address
		//third byte is memory location to write to
		//fourth byte is data to write

		int addr = command[1];
		int memAddr = command[2];
		int writeByte = command[3];
		Serial.print("Sending ");
		Serial.print(writeByte);
		Serial.print(" to device ");
		Serial.print(addr);
		Serial.print(" memory address: ");
		Serial.println(memAddr);

		Wire.beginTransmission(command[1]);
		Wire.write(command[2]);

		for(int i = 3; i < size+1; i++){
			Wire.write(command[i]);
		}
		byte status = Wire.endTransmission();
		if(status != 0){
			//Error

			return;
		}else{
			//Set command success
		}
	}
}

void doBroadcast(){
	udp.beginPacket(broadcastIP, remotePort);
	udp.print(mac);
	return;
}

void sendHTTPRequest(char message[], String action){
//	if (writeReq==NREQS) {
//		writeReq=0;
//	}
	//tries to send the request up to 2 times until an HTTP response status code of 200 is received.
	sends++;
	Serial.print("Sends: ");
	Serial.println(sends);
	bool tryAgain = false;
//	retry:

	//setup HTTP Request
	request.hostname = "cloud.controlanything.com";
	request.port = 420;
	request.path = "/device/"+deviceIDString+"/"+action+"/"+version +"/" +"?data="+(const char*)message;

	//Send request
	http.get(request, response, headers);
	lastCheckIn = millis();

	int rStatus = response.status;

	Serial.print("Response Status = ");
	Serial.println(rStatus);

	if(rStatus != 200){
		init = true;
		return;
	}
	Serial.print("Length of response from server: ");
	Serial.println(response.responseLength);
	String serverCheckInResponse = aesClient.cbcDecrypt(response.body);
	Serial.print("Decrypted: ");
	Serial.println(serverCheckInResponse);
	if(serverCheckInResponse.equals("Fail")){
		init = true;
		return;
	}else{
		//Good message from server
		parseServerResponse(serverCheckInResponse);
//		if(sends % 2 == 0){
//			init = true;
//		}
	}
	return;
}

void parseServerResponse(String message){

	char messageArray[message.length()+1];
	message.toCharArray(messageArray, sizeof messageArray, 0);

	StaticJsonBuffer<2056> jsonBuffer;
	JsonArray& commandRoot = jsonBuffer.parseArray(messageArray);

	if(!commandRoot.success()){
		Serial.println("Parse failed");
	}else{
//		//Run Commands
		JsonArray& runCommands = commandRoot[0].asArray();
		numberOfRunCommands = 0;
		for(int i = 0; i < runCommands.size(); i++){
			JsonArray& runCommand = runCommands[i];
			if(runCommand.size() > 1){
				unsigned char command[runCommand.size()+1];
				command[0] = runCommand.size();
				for(int j = 0; j < sizeof command; j++){
					command[j+1] = runCommand[j];
				}

				if(i < 16){
					memcpy(runCommandsArray[i], command, sizeof command);
				}
				numberOfRunCommands++;
			}else{
				Serial.print("Run command size is: ");
				Serial.println(runCommand.size());
			}
		}
		Serial.print("Number of run commands: ");
		Serial.println(numberOfRunCommands);


		//Monitor Commands
//		JsonArray& monitorCommands = commandRoot[1].asArray();

		//Report Commands
		JsonArray& reportCommands = commandRoot[2].asArray();
		numberOfReportCommands = 0;
		for(int i = 0; i < reportCommands.size(); i++){
			JsonArray& reportCommand = reportCommands[i];
			if(reportCommand.size() > 1){
				unsigned char rCommand[reportCommand.size()];
				//Set first byte of command to length of reportCommand
				for(int j = 0; j < sizeof rCommand; j++){
					rCommand[j] = reportCommand[j];
				}
				if(i < 16){
					memcpy(reportCommandsArray[i], rCommand, sizeof rCommand);
					numberOfReportCommands++;
				}
			}

		}
		Serial.print("Number of report commands: ");
		Serial.println(numberOfReportCommands);
	}
	return;

 }

void processRunCommands(){
	Wire.begin();
	int numerOfRunCommandsProcessed = 0;
	int numberOfFails = 0;
	for(int i = 0; i < numberOfRunCommands; i++){
		//Open connection to chip i2c bus address
		Wire.beginTransmission(runCommandsArray[i][1]);
		//State the begin write address
		Wire.write(runCommandsArray[i][2]);
		//Write the bytes to the chip
		for(int j = 3; j < runCommandsArray[i][0]+1; j++){
			Wire.write(runCommandsArray[i][j]);
		}
		byte status = Wire.endTransmission();
		if(status != 0){
			numberOfFails++;
		}else{
			numerOfRunCommandsProcessed++;
		}
		delay(50);
	}
	Serial.print("Number of run commands processed: ");
	Serial.println(numerOfRunCommandsProcessed);
	Serial.print("Number of run commands failed: ");
	Serial.println(numberOfFails);
	numberOfRunCommands = 0;
	return;

}

void processReportCommands(){
	//First byte in packet is I2C Bus Address
	//Second byte in packet is length of write bytes
	//Thrid byte in packet is length of bytes to read
	//Next bytes (if applicable) are write bytes

	Wire.begin();
	int numberOfReportCommandsProcessed = 0;
	for(int i = 0; i < numberOfReportCommands; i++){
		Wire.beginTransmission(reportCommandsArray[i][0]);
		for(int j = 3; j < reportCommandsArray[i][1]+3; j++){
			Wire.write(reportCommandsArray[i][j]);
		}
				byte status = Wire.endTransmission();
		if(status != 0){
			Serial.println("chip did not respond to report command sent");
			memcpy(reportBytes[i], commandFailed, sizeof commandFailed);
		}else{
			int length = Wire.requestFrom(reportCommandsArray[i][0], reportCommandsArray[i][2]);
			if(length != reportCommandsArray[i][2]){
				Serial.println("chip did not respond with proper number of bytes sent in report command");
				memcpy(reportBytes[i], commandFailed, sizeof commandFailed);
			}else{
				numOfBytesInReport[i] = length;
				for(int j = 0; j < length; j++){
					reportBytes[i][j] = Wire.read();
				}
			}
		}
		numberOfReportCommandsProcessed++;
	}
	return;
//	Serial.print("number of report commands processed: ");
//	Serial.println(numberOfReportCommandsProcessed);
}

String hexString(unsigned char data[],  size_t len){
	const char hex[] = "0123456789ABCDEF";
	String s;
	for(size_t i = 0; i < len; i++){
		char c = data[i];
		char hexDigit = hex[(c >> 4) & 0xF];
		s += hexDigit+" ";
	}
	return s;
}

void getMac(){
	byte macBytesRevers[6];
	WiFi.macAddress(macBytesRevers);
	sprintf(mac, "%02x%02x%02x%02x%02x%02x;%d;", macBytesRevers[5], macBytesRevers[4], macBytesRevers[3], macBytesRevers[2], macBytesRevers[1], macBytesRevers[0], port);
	return;
}
