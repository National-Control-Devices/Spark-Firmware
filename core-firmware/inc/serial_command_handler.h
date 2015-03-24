/*
 * serial_command_handler.h
 *
 *  Created on: Mar 9, 2015
 *      Author: travis
 */

#ifndef SERIAL_COMMAND_HANDLER_H_
#define SERIAL_COMMAND_HANDLER_H_

#include "spark_wiring_usbserial.h"
#include "spark_wiring_eeprom.h"

class serial_command_handler{
public:
	//Constructor
	serial_command_handler(void);
	void checkPort(void);
};



#endif /* SERIAL_COMMAND_HANDLER_H_ */
