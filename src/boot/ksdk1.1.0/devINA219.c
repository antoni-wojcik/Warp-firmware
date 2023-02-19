
#include <stdlib.h>

/*
 *	config.h needs to come first
 */
#include "config.h"

#include "fsl_misc_utilities.h"
#include "fsl_device_registers.h"
#include "fsl_i2c_master_driver.h"
#include "fsl_spi_master_driver.h"
#include "fsl_rtc_driver.h"
#include "fsl_clock_manager.h"
#include "fsl_power_manager.h"
#include "fsl_mcglite_hal.h"
#include "fsl_port_hal.h"

#include "gpio_pins.h"
#include "SEGGER_RTT.h"
#include "warp.h"


extern volatile WarpI2CDeviceState	deviceINA219State;
extern volatile uint32_t		gWarpI2cBaudRateKbps;
extern volatile uint32_t		gWarpI2cTimeoutMilliseconds;
extern volatile uint32_t		gWarpSupplySettlingDelayMilliseconds;



void
initINA219(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts)
{
	deviceINA219State.i2cAddress			= i2cAddress;
	deviceINA219State.operatingVoltageMillivolts	= operatingVoltageMillivolts;

	return;
}

WarpStatus
writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payload)
{
	uint8_t		payloadByte[2], commandByte[1];
	i2c_status_t	status;

	switch (deviceRegister)
	{
		case 0x00: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
	commandByte[0] = deviceRegister;
	payloadByte[0] = (payload >> 8) & 0xFF;
    payloadByte[1] = (payload     ) & 0xFF;
	warpEnableI2Cpins();

	status = I2C_DRV_MasterSendDataBlocking(
							0 /* I2C instance */,
							&slave,
							commandByte,
							1,
							payloadByte,
							2, /* 2 bytes */
							gWarpI2cTimeoutMilliseconds);
	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

WarpStatus
configureSensorINA219(uint16_t payloadCONF, uint16_t payloadCAL)
{
	WarpStatus	i2cWriteStatus1, i2cWriteStatus2;


	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);

	i2cWriteStatus1 = writeSensorRegisterINA219(kWarpSensorConfigurationRegisterINA219_CONF /* register address CONFIGURATION */,
							payloadCONF/* payload: Configure the sensor */
							);

	i2cWriteStatus2 = writeSensorRegisterINA219(kWarpSensorConfigurationRegisterINA219_CAL /* register address CALIBRATION */,
							payloadCAL /* payload: Calibrate the sensor for current measuremnts */
							);

	warpPrint("\r\n\tINA219 INITIALIZATION: %d, %d\n", i2cWriteStatus1, i2cWriteStatus2);

	return (i2cWriteStatus1 | i2cWriteStatus2);
}

WarpStatus
readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes)
{
	uint8_t		    cmdBuf[1] = {0xFF};
	i2c_status_t	status;


	USED(numberOfBytes);
	switch (deviceRegister)
	{
		case 0x00: case 0x01: case 0x02: 
        case 0x03: case 0x04: case 0x05:
		{
			/* OK */
			break;
		}
		
		default:
		{
			return kWarpStatusBadDeviceCommand;
		}
	}

	i2c_device_t slave =
	{
		.address = deviceINA219State.i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
	cmdBuf[0] = deviceRegister;
	warpEnableI2Cpins();

	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							cmdBuf,
							1,
							(uint8_t *)deviceINA219State.i2cBuffer,
							numberOfBytes,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

static WarpStatus
tryI2CSlaveReadSensorRegister(uint8_t i2cAddress)
{
	uint8_t		commandByte[1] = {0x00};
	i2c_status_t	status;

	i2c_device_t slave =
	{
		.address = i2cAddress,
		.baudRate_kbps = gWarpI2cBaudRateKbps
	};

	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);
	warpEnableI2Cpins();

	status = I2C_DRV_MasterReceiveDataBlocking(
							0 /* I2C peripheral instance */,
							&slave,
							commandByte,
							1,
							(uint8_t *)deviceINA219State.i2cBuffer,
							2,
							gWarpI2cTimeoutMilliseconds);

	if (status != kStatus_I2C_Success)
	{
		return kWarpStatusDeviceCommunicationFailed;
	}

	return kWarpStatusOK;
}

void
printAllSensorDataINA219(bool hexModeFlag)
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	int16_t		readSensorRegisterValueCombined;
	WarpStatus	i2cReadStatus;

	int16_t 	shuntVoltage;
	uint16_t	busVoltage;
	int16_t 	current;


	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);


    /* Read the shunt voltage. With the With the CONFIGURATION REGISTER set to 0x399F, the shunt voltage LSB is 0.01 mV */
	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_SHUNT, 2 /* numberOfBytes */);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
	readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
	readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | readSensorRegisterValueLSB;

	if (hexModeFlag)
	{
		warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
	}

    shuntVoltage = readSensorRegisterValueCombined; 

    /* Read the bus voltage. The bus voltage LSB is always 4mV */
    i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_BUS, 2 /* numberOfBytes */);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
	readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
	readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | readSensorRegisterValueLSB;\

	if (hexModeFlag)
	{
		warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
	}

    bool busVoltageOverflow = readSensorRegisterValueLSB & 0x01;
//  bool convReadyBit = readSensorRegisterValueLSB & 0x02;
    busVoltage = readSensorRegisterValueCombined >> 3;

	/* Read the current. With the CALIBRATION REGISTER set to 0x4FFF, the current LSB is 20 uA */
	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_CURRENT, 2 /* numberOfBytes */);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
	readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
	readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | readSensorRegisterValueLSB;

	if (hexModeFlag)
	{
		warpPrint(" 0x%02x 0x%02x,", readSensorRegisterValueMSB, readSensorRegisterValueLSB);
	}

    current = readSensorRegisterValueCombined;


	if (i2cReadStatus != kWarpStatusOK)
	{
		warpPrint(" ----, ----, ----, ----,");
	}
	else
	{
		if (!hexModeFlag)
		{
			int16_t resistanceInt = (int16_t)((float)(busVoltage) / (float)(current) * 200.0f); /* times 200 because of the factors from LSBs */

            if (busVoltageOverflow)
            {
                warpPrint(" %d, (BUS VOLTAGE OVERFLOW) %u, %d, %d,", shuntVoltage, busVoltage, current, resistanceInt);
            }
            else
            {
			    warpPrint(" %d, %u, %d, %d,", shuntVoltage, busVoltage, current, resistanceInt);
            }
		}
	}
}

void
printCurrentMicroamperesINA219()
{
	uint16_t	readSensorRegisterValueLSB;
	uint16_t	readSensorRegisterValueMSB;
	int16_t		readSensorRegisterValueCombined;
	WarpStatus	i2cReadStatus;

	int16_t 	currentMicroamperes;


	warpScaleSupplyVoltage(deviceINA219State.operatingVoltageMillivolts);


	/* Read the current. With the CALIBRATION REGISTER set to 0x4FFF, the current LSB is 20 uA */
	i2cReadStatus = readSensorRegisterINA219(kWarpSensorOutputRegisterINA219_CURRENT, 2 /* numberOfBytes */);
	readSensorRegisterValueMSB = deviceINA219State.i2cBuffer[0];
	readSensorRegisterValueLSB = deviceINA219State.i2cBuffer[1];
	readSensorRegisterValueCombined = ((readSensorRegisterValueMSB & 0xFF) << 8) | readSensorRegisterValueLSB;

	currentMicroamperes = readSensorRegisterValueCombined * 20; // raw data * 20 uA


	if (i2cReadStatus != kWarpStatusOK)
	{
		warpPrint(" ----,");
	}
	else
	{
		warpPrint(" %d,", currentMicroamperes);
	}
}

/* Repeat current measurements n times */
void
repeatPrintCurrentMicroamperesINA219(int nTimes)
{
	configureSensorINA219(0x399F, /* Set the configuration register */
						  0x4FFF  /* Calibrate the sensor for current measurement */
						  );
	
	warpPrint("\r");
	for(int i = 0; i < 1000; i++) {
		printCurrentMicroamperesINA219();
	}
	warpPrint("\n");
}

/* Utility to find all detected I2C slave addresses and list them */
void
searchOverI2CSlaveAddresses() {
	for (uint8_t i = 0x00; i <= 0x7F; i++) {
		WarpStatus status = tryI2CSlaveReadSensorRegister(i);

		if(!status)
		{
			warpPrint("\rI2C slave address 0x%02X found\n", i);

			/* Print all sensor data if sensor detected */
			/*
			if(i == 0x40)
			{
				warpPrint("\r");
				printAllSensorDataINA219(false);
				warpPrint("\n);
			}
			*/
		}
	}
}