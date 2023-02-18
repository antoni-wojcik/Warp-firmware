// Header file for the INA291 driver

void		initINA291(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts);
WarpStatus	readSensorRegisterINA291(uint8_t deviceRegister, int numberOfBytes);
WarpStatus	writeSensorRegisterINA291(uint8_t deviceRegister, uint8_t payloadBtye);
WarpStatus	configureSensorINA291(uint8_t payloadF_SETUP, uint8_t payloadCTRL_REG1);
void		printSensorDataINA291(bool hexModeFlag);