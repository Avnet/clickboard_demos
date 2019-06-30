

/**
* @brief  Function to read LSM6DSL data using the method HW requires.
* @param  pBuffer: pointer to data to be read.
* @param  RegisterAddr: specifies internal address register to be read.
* @param  NumByteToRead: number of bytes to be read.
* @retval 0 if ok, an error code otherwise.
*/
uint8_t (*IO_Read)(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToRead);
//{
//      dev_i2c->beginTransmission(((uint8_t)(((address) >> 1) & 0x7F)));
//      dev_i2c->write(RegisterAddr);
//      dev_i2c->endTransmission(false);
//
//      dev_i2c->requestFrom(((uint8_t)(((address) >> 1) & 0x7F)), (byte) NumByteToRead);
//
//      int i=0;
//      while (dev_i2c->available())
//      {
//        pBuffer[i] = dev_i2c->read();
//        i++;
//      }
//
//      return 0;
//}
    
/**
* @brief  Function to write data to the LSM6DSL using the method HW requires.
* @param  pBuffer: pointer to data to be written.
* @param  RegisterAddr: specifies internal address register to be written.
* @param  NumByteToWrite: number of bytes to write.
* @retval 0 if ok, an error code otherwise.
*/
uint8_t (*IO_Write)(uint8_t* pBuffer, uint8_t RegisterAddr, uint16_t NumByteToWrite);
//{
//    dev_i2c->beginTransmission(((uint8_t)(((address) >> 1) & 0x7F)));
//
//    dev_i2c->write(RegisterAddr);
//    for (int i = 0 ; i < NumByteToWrite ; i++)
//        dev_i2c->write(pBuffer[i]);
//
//    dev_i2c->endTransmission(true);
//
//    return 0;
//}

uint8_t LSM6DSL_IO_Write( void *handle, uint8_t WriteAddr, uint8_t *pBuffer, uint16_t nBytesToWrite );
uint8_t LSM6DSL_IO_Read( void *handle, uint8_t ReadAddr, uint8_t *pBuffer, uint16_t nBytesToRead );

#endif

