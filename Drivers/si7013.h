/***************************************************************************//**
 * @file
 * @brief Driver for the Si7013 Temperature / Humidity sensor
 * @version 3.20.12
 *******************************************************************************
 * @section License
 * <b>(C) Copyright 2014 Silicon Labs, http://www.silabs.com</b>
 *******************************************************************************
 *
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 *
 ******************************************************************************/

#ifndef __SI7013_H
#define __SI7013_H

#include "em_device.h"
#include <stdbool.h>

/***************************************************************************//**
 * @addtogroup Drivers
 * @{
 ******************************************************************************/

/***************************************************************************//**
 * @addtogroup Si7013
 * @{
 ******************************************************************************/

#ifdef __cplusplus
extern "C" {
#endif

/*******************************************************************************
 *******************************   DEFINES   ***********************************
 ******************************************************************************/

/** I2C device address for Si7013 */
#define SI7013_ADDR      0x82
/** I2C device address for Si7021 */
#define SI7021_ADDR      0x80


/** Device ID value for Si7013 */
#define SI7013_DEVICE_ID 0x0D
/** Device ID value for Si7020 */
#define SI7020_DEVICE_ID 0x14
/** Device ID value for Si7021 */
#define SI7021_DEVICE_ID 0x21

/*******************************************************************************
 *****************************   PROTOTYPES   **********************************
 ******************************************************************************/

int32_t Si7013_MeasureRHAndTemp(I2C_TypeDef *i2c, uint8_t addr,
                                 uint32_t *rhData, int32_t *tData);

int32_t Si7013_GetFirmwareRevision(I2C_TypeDef *i2c, uint8_t addr, uint8_t *fwRev);
#define I2C_FLAG_WRITE_READ     0x0004
bool Si7013_Detect(I2C_TypeDef *i2c, uint8_t addr, uint8_t *deviceId);
static int32_t Si7013_Measure(I2C_TypeDef *i2c, uint8_t addr, uint32_t *data,
                              uint8_t command){
	  I2C_TransferSeq_TypeDef    seq;
	  I2C_TransferReturn_TypeDef ret;
	  uint8_t                    i2c_read_data[2];
	  uint8_t                    i2c_write_data[1];

	  seq.addr  = addr;
	  seq.flags = I2C_FLAG_WRITE_READ;
	  /* Select command to issue */
	  i2c_write_data[0] = command;
	  seq.buf[0].data   = i2c_write_data;
	  seq.buf[0].len    = 1;
	  /* Select location/length of data to be read */
	  seq.buf[1].data = i2c_read_data;
	  seq.buf[1].len  = 2;

	  ret = I2CSPM_Transfer(i2c, &seq);

	  if (ret != i2cTransferDone)
	  {
	    *data = 0;
	    return((int) ret);
	  }

	  *data = ((uint32_t) i2c_read_data[0] << 8) + (i2c_read_data[1] & 0xfc);

	  return((int) 2);
	}

#ifdef __cplusplus
}
#endif

/** @} (end group Si7013) */
/** @} (end group Drivers) */
#endif /* __SI7013_H */
