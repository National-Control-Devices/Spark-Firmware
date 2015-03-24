/*
 * i2c_handler.h
 *
 *  Created on: Mar 10, 2015
 *      Author: travis
 */

#ifndef I2C_HANDLER_H_
#define I2C_HANDLER_H_

#include "spark_wiring_i2c.h"

class i2c_handler{
public:
	//constructor
	i2c_handler(void);
	unsigned char write(unsigned char* buffer, size_t bufferSize);
	unsigned char* read(unsigned char* buffer, size_t bufferSize);
	bool checkIsRead(unsigned char* buffer, size_t bufferSize);
	unsigned char setAddress(unsigned char addr);
};



#endif /* I2C_HANDLER_H_ */
