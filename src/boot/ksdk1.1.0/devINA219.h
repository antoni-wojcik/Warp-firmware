// Header file for the INA291 driver

void		initINA219(const uint8_t i2cAddress, uint16_t operatingVoltageMillivolts);
WarpStatus	readSensorRegisterINA219(uint8_t deviceRegister, int numberOfBytes);
WarpStatus	writeSensorRegisterINA219(uint8_t deviceRegister, uint16_t payloadBtye);
WarpStatus	configureSensorINA219(uint16_t payloadCONF, uint16_t payloadCAL);
void		printAllSensorDataINA219(bool hexModeFlag);
void		printCurrentMicroamperesINA219();
void        repeatPrintCurrentMicroamperesINA219(int nTimes);
void        searchOverI2CSlaveAddresses();