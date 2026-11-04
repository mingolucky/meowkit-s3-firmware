/******************************************************************************
  * \attention
  *
  * <h2><center>&copy; COPYRIGHT 2021 STMicroelectronics</center></h2>
  *
  * Licensed under ST MIX MYLIBERTY SOFTWARE LICENSE AGREEMENT (the "License");
  * You may not use this file except in compliance with the License.
  * You may obtain a copy of the License at:
  *
  *        www.st.com/mix_myliberty
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied,
  * AND SPECIFICALLY DISCLAIMING THE IMPLIED WARRANTIES OF MERCHANTABILITY,
  * FITNESS FOR A PARTICULAR PURPOSE, AND NON-INFRINGEMENT.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  *
******************************************************************************/

/*! \file
 *
 *  \author SRA
 *
 *  \brief Implementation of ST25R3916 communication.
 *
 */

/*
******************************************************************************
* INCLUDES
******************************************************************************
*/
#include "rfal_rfst25r3916.h"
#include "st25r3916_com.h"
#include "st25r3916.h"
#include "nfc_utils.h"


/*
******************************************************************************
* LOCAL DEFINES
******************************************************************************
*/

#define ST25R3916_OPTIMIZE              true                           /*!< Optimization switch: false always write value to register      */
#define ST25R3916_I2C_ADDR              (0xA0U >> 1)                   /*!< ST25R3916's default I2C address                                */
#define ST25R3916_REG_LEN               1U                             /*!< Byte length of a ST25R3916 register                            */

#define ST25R3916_WRITE_MODE            (0U << 6)                      /*!< ST25R3916 Operation Mode: Write                                */
#define ST25R3916_READ_MODE             (1U << 6)                      /*!< ST25R3916 Operation Mode: Read                                 */
#define ST25R3916_CMD_MODE              (3U << 6)                      /*!< ST25R3916 Operation Mode: Direct Command                       */
#define ST25R3916_FIFO_LOAD             (0x80U)                        /*!< ST25R3916 Operation Mode: FIFO Load                            */
#define ST25R3916_FIFO_READ             (0x9FU)                        /*!< ST25R3916 Operation Mode: FIFO Read                            */
#define ST25R3916_PT_A_CONFIG_LOAD      (0xA0U)                        /*!< ST25R3916 Operation Mode: Passive Target Memory A-Config Load  */
#define ST25R3916_PT_F_CONFIG_LOAD      (0xA8U)                        /*!< ST25R3916 Operation Mode: Passive Target Memory F-Config Load  */
#define ST25R3916_PT_TSN_DATA_LOAD      (0xACU)                        /*!< ST25R3916 Operation Mode: Passive Target Memory TSN Load       */
#define ST25R3916_PT_MEM_READ           (0xBFU)                        /*!< ST25R3916 Operation Mode: Passive Target Memory Read           */

#define ST25R3916_CMD_LEN               (1U)                           /*!< ST25R3916 CMD length                                           */
#define ST25R3916_BUF_LEN               (ST25R3916_CMD_LEN+ST25R3916_FIFO_DEPTH) /*!< ST25R3916 communication buffer: CMD + FIFO length    */

/*
******************************************************************************
* LOCAL VARIABLES
******************************************************************************
*/

/*
******************************************************************************
* LOCAL FUNCTION PROTOTYPES
******************************************************************************
*/


/*
******************************************************************************
* GLOBAL FUNCTIONS
******************************************************************************
*/
/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ReadRegister(uint8_t reg, uint8_t *val)
{
  return st25r3916ReadMultipleRegisters(reg, val, ST25R3916_REG_LEN);
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ReadMultipleRegisters(uint8_t reg, uint8_t *values, uint8_t length)
{
  if (length && !values) return ERR_PARAM;
  if (length) ST_MEMSET(values, 0, length);
  if (length > 0U) {
    if (!i2c_enabled) {
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      /* If is a space-B register send a direct command first */
      if ((reg & ST25R3916_SPACE_B) != 0U) {
        dev_spi->transfer(ST25R3916_CMD_SPACE_B_ACCESS);
      }

      dev_spi->transfer(((reg & ~ST25R3916_SPACE_B) | ST25R3916_READ_MODE));
      dev_spi->transfer((void *)values, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));
      /* If is a space-B register send a direct command first */
      if ((reg & ST25R3916_SPACE_B) != 0U) {
        dev_i2c->write(ST25R3916_CMD_SPACE_B_ACCESS);
      }

      dev_i2c->write((reg & ~ST25R3916_SPACE_B) | ST25R3916_READ_MODE);
      dev_i2c->endTransmission(false);
      dev_i2c->requestFrom(((uint8_t)(ST25R3916_I2C_ADDR & 0x7F)), (size_t) length);

      int i = 0;
      while (dev_i2c->available()) {
        values[i] = dev_i2c->read();
        i++;
      }
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WriteRegister(uint8_t reg, uint8_t val)
{
  uint8_t value = val;               /* MISRA 17.8: use intermediate variable */
  return st25r3916WriteMultipleRegisters(reg, &value, ST25R3916_REG_LEN);
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WriteMultipleRegisters(uint8_t reg, const uint8_t *values, uint8_t length)
{
  if (length && !values) return ERR_PARAM;
  if (length > 0U) {
    if (!i2c_enabled) {
      uint8_t  tx[ST25R3916_FIFO_DEPTH];

      if (values != NULL) {
        (void)memcpy(tx, values, length);
      }
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      /* If is a space-B register send a direct command first */
      if ((reg & ST25R3916_SPACE_B) != 0U) {
        dev_spi->transfer(ST25R3916_CMD_SPACE_B_ACCESS);
      }

      dev_spi->transfer(((reg & ~ST25R3916_SPACE_B) | ST25R3916_WRITE_MODE));
      dev_spi->transfer((void *)tx, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));
      /* If is a space-B register send a direct command first */
      if ((reg & ST25R3916_SPACE_B) != 0U) {
        dev_i2c->write(ST25R3916_CMD_SPACE_B_ACCESS);
      }

      dev_i2c->write((reg & ~ST25R3916_SPACE_B) | ST25R3916_WRITE_MODE);

      for (uint16_t i = 0 ; i < length ; i++) {
        dev_i2c->write(values[i]);
      }
      dev_i2c->endTransmission(true);
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WriteFifo(const uint8_t *values, uint16_t length)
{
  if (length && !values) return ERR_PARAM;
  if (length > ST25R3916_FIFO_DEPTH) {
    return ERR_PARAM;
  }

  if (length > 0U) {
    if (!i2c_enabled) {
      uint8_t  tx[ST25R3916_FIFO_DEPTH];

      if (values != NULL) {
        (void)memcpy(tx, values, length);
      }
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_FIFO_LOAD);

      dev_spi->transfer((void *)tx, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

      dev_i2c->write(ST25R3916_FIFO_LOAD);

      for (uint16_t i = 0 ; i < length ; i++) {
        dev_i2c->write(values[i]);
      }
      dev_i2c->endTransmission(true);
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ReadFifo(uint8_t *buf, uint16_t length)
{
  if (length > ST25R3916_FIFO_DEPTH) return ERR_PARAM;
  // RFAL deliberately passes NULL to drain bytes beyond the caller's buffer.
  uint8_t discard[ST25R3916_FIFO_DEPTH];
  if (!buf) buf = discard;
  if (length) ST_MEMSET(buf, 0, length);
  if (length > 0U) {
    if (!i2c_enabled) {
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_FIFO_READ);

      dev_spi->transfer((void *)buf, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));
      dev_i2c->write(ST25R3916_FIFO_READ);
      dev_i2c->endTransmission(false);
      dev_i2c->requestFrom(((uint8_t)(ST25R3916_I2C_ADDR & 0x7F)), (size_t) length);

      int i = 0;
      while (dev_i2c->available()) {
        buf[i] = dev_i2c->read();
        i++;
      }
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WritePTMem(const uint8_t *values, uint16_t length)
{
  if (length > ST25R3916_PTM_LEN) {
    return ERR_PARAM;
  }

  if (length > 0U) {
    if (!i2c_enabled) {
      uint8_t  tx[ST25R3916_FIFO_DEPTH];

      if (values != NULL) {
        (void)memcpy(tx, values, length);
      }
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_PT_A_CONFIG_LOAD);

      dev_spi->transfer((void *)tx, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

      dev_i2c->write(ST25R3916_PT_A_CONFIG_LOAD);

      for (uint16_t i = 0 ; i < length ; i++) {
        dev_i2c->write(values[i]);
      }
      dev_i2c->endTransmission(true);
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ReadPTMem(uint8_t *values, uint16_t length)
{
  uint8_t tmp[ST25R3916_REG_LEN + ST25R3916_PTM_LEN] = {};  /* local buffer to handle prepended byte on I2C and SPI */

  if (length > 0U) {
    if (length > ST25R3916_PTM_LEN) {
      return ERR_PARAM;
    }


    if (!i2c_enabled) {
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_PT_MEM_READ);

      dev_spi->transfer((void *)tmp, (ST25R3916_REG_LEN + length));

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));
      dev_i2c->write(ST25R3916_PT_MEM_READ);
      dev_i2c->endTransmission(false);
      dev_i2c->requestFrom(((uint8_t)(ST25R3916_I2C_ADDR & 0x7F)), (size_t)(ST25R3916_REG_LEN + length));

      int i = 0;
      while (dev_i2c->available()) {
        tmp[i] = dev_i2c->read();
        i++;
      }
    }

    /* Copy PTMem content without prepended byte */
    ST_MEMCPY(values, (tmp + ST25R3916_REG_LEN), length);

    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WritePTMemF(const uint8_t *values, uint16_t length)
{
  if (length > (ST25R3916_PTM_F_LEN + ST25R3916_PTM_TSN_LEN)) {
    return ERR_PARAM;
  }

  if (length > 0U) {
    if (!i2c_enabled) {
      uint8_t  tx[ST25R3916_FIFO_DEPTH];

      if (values != NULL) {
        (void)memcpy(tx, values, length);
      }
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_PT_F_CONFIG_LOAD);

      dev_spi->transfer((void *)tx, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

      dev_i2c->write(ST25R3916_PT_F_CONFIG_LOAD);

      for (uint16_t i = 0 ; i < length ; i++) {
        dev_i2c->write(values[i]);
      }
      dev_i2c->endTransmission(true);
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WritePTMemTSN(const uint8_t *values, uint16_t length)
{
  if (length > ST25R3916_PTM_TSN_LEN) {
    return ERR_PARAM;
  }

  if (length > 0U) {
    if (!i2c_enabled) {
      uint8_t  tx[ST25R3916_FIFO_DEPTH];

      if (values != NULL) {
        (void)memcpy(tx, values, length);
      }
      /* Setting Transaction Parameters */
      dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
      digitalWrite(cs_pin, LOW);

      dev_spi->transfer(ST25R3916_PT_TSN_DATA_LOAD);

      dev_spi->transfer((void *)tx, length);

      digitalWrite(cs_pin, HIGH);
      dev_spi->endTransaction();
    } else {
      dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

      dev_i2c->write(ST25R3916_PT_TSN_DATA_LOAD);

      for (uint16_t i = 0 ; i < length ; i++) {
        dev_i2c->write(values[i]);
      }
      dev_i2c->endTransmission(true);
    }
    if (isr_pending) {
      st25r3916Isr();
      isr_pending = false;
    }
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ExecuteCommand(uint8_t cmd)
{
  if (!i2c_enabled) {
    /* Setting Transaction Parameters */
    dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
    digitalWrite(cs_pin, LOW);

    dev_spi->transfer((cmd | ST25R3916_CMD_MODE));

    digitalWrite(cs_pin, HIGH);
    dev_spi->endTransaction();
  } else {
    dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

    dev_i2c->write((cmd | ST25R3916_CMD_MODE));

    dev_i2c->endTransmission(true);
  }

  if (isr_pending) {
    st25r3916Isr();
    isr_pending = false;
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ReadTestRegister(uint8_t reg, uint8_t *val)
{
  if (!i2c_enabled) {
    /* Setting Transaction Parameters */
    dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
    digitalWrite(cs_pin, LOW);

    dev_spi->transfer(ST25R3916_CMD_TEST_ACCESS);
    dev_spi->transfer((reg | ST25R3916_READ_MODE));

    dev_spi->transfer((void *)val, ST25R3916_REG_LEN);

    digitalWrite(cs_pin, HIGH);
    dev_spi->endTransaction();
  } else {
    dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

    dev_i2c->write(ST25R3916_CMD_TEST_ACCESS);
    dev_i2c->write((reg | ST25R3916_READ_MODE));
    dev_i2c->endTransmission(false);
    dev_i2c->requestFrom(((uint8_t)(ST25R3916_I2C_ADDR & 0x7F)), ST25R3916_REG_LEN);

    if (dev_i2c->available()) {
      *val = dev_i2c->read();
    }
  }

  if (isr_pending) {
    st25r3916Isr();
    isr_pending = false;
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916WriteTestRegister(uint8_t reg, uint8_t val)
{

  if (!i2c_enabled) {
    uint8_t value = val;               /* MISRA 17.8: use intermediate variable */
    /* Setting Transaction Parameters */
    dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));
    digitalWrite(cs_pin, LOW);

    dev_spi->transfer(ST25R3916_CMD_TEST_ACCESS);

    dev_spi->transfer((reg | ST25R3916_WRITE_MODE));

    dev_spi->transfer((void *)&value, ST25R3916_REG_LEN);

    digitalWrite(cs_pin, HIGH);
    dev_spi->endTransaction();
  } else {
    dev_i2c->beginTransmission((uint8_t)(ST25R3916_I2C_ADDR & 0x7F));

    dev_i2c->write(ST25R3916_CMD_TEST_ACCESS);

    dev_i2c->write((reg | ST25R3916_WRITE_MODE));

    dev_i2c->write(val);

    dev_i2c->endTransmission(true);
  }

  if (isr_pending) {
    st25r3916Isr();
    isr_pending = false;
  }

  return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ClrRegisterBits(uint8_t reg, uint8_t clr_mask)
{
  ReturnCode ret;
  uint8_t    rdVal;

  /* Read current reg value */
  EXIT_ON_ERR(ret, st25r3916ReadRegister(reg, &rdVal));

  /* Only perform a Write if value to be written is different */
  if (ST25R3916_OPTIMIZE && (rdVal == (uint8_t)(rdVal & ~clr_mask))) {
    return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
  }

  /* Write new reg value */
  return st25r3916WriteRegister(reg, (uint8_t)(rdVal & ~clr_mask));
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916SetRegisterBits(uint8_t reg, uint8_t set_mask)
{
  ReturnCode ret;
  uint8_t    rdVal;

  /* Read current reg value */
  EXIT_ON_ERR(ret, st25r3916ReadRegister(reg, &rdVal));

  /* Only perform a Write if the value to be written is different */
  if (ST25R3916_OPTIMIZE && (rdVal == (rdVal | set_mask))) {
    return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
  }

  /* Write new reg value */
  return st25r3916WriteRegister(reg, (rdVal | set_mask));
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ChangeRegisterBits(uint8_t reg, uint8_t valueMask, uint8_t value)
{
  return st25r3916ModifyRegister(reg, valueMask, (valueMask & value));
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ModifyRegister(uint8_t reg, uint8_t clr_mask, uint8_t set_mask)
{
  ReturnCode ret;
  uint8_t    rdVal;
  uint8_t    wrVal;

  /* Read current reg value */
  EXIT_ON_ERR(ret, st25r3916ReadRegister(reg, &rdVal));

  /* Compute new value */
  wrVal  = (uint8_t)(rdVal & ~clr_mask);
  wrVal |= set_mask;

  /* Only perform a Write if the value to be written is different */
  if (ST25R3916_OPTIMIZE && (rdVal == wrVal)) {
    return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
  }

  /* Write new reg value */
  return st25r3916WriteRegister(reg, wrVal);
}


/*******************************************************************************/
ReturnCode RfalRfST25R3916Class::st25r3916ChangeTestRegisterBits(uint8_t reg, uint8_t valueMask, uint8_t value)
{
  ReturnCode ret;
  uint8_t    rdVal;
  uint8_t    wrVal;

  /* Read current reg value */
  EXIT_ON_ERR(ret, st25r3916ReadTestRegister(reg, &rdVal));

  /* Compute new value */
  wrVal  = (uint8_t)(rdVal & ~valueMask);
  wrVal |= (uint8_t)(value & valueMask);

  /* Only perform a Write if the value to be written is different */
  if (ST25R3916_OPTIMIZE && (rdVal == wrVal)) {
    return (i2c_enabled && !dev_i2c->ok()) ? ERR_IO : ERR_NONE;
  }

  /* Write new reg value */
  return st25r3916WriteTestRegister(reg, wrVal);
}


/*******************************************************************************/
bool RfalRfST25R3916Class::st25r3916CheckReg(uint8_t reg, uint8_t mask, uint8_t val)
{
  uint8_t regVal;

  regVal = 0;
  st25r3916ReadRegister(reg, &regVal);

  return ((regVal & mask) == val);
}


/*******************************************************************************/
bool RfalRfST25R3916Class::st25r3916IsRegValid(uint8_t reg)
{
  if (!(((int16_t)reg >= (int16_t)ST25R3916_REG_IO_CONF1) && (reg <= (ST25R3916_SPACE_B | ST25R3916_REG_IC_IDENTITY)))) {
    return false;
  }
  return true;
}

/*
******************************************************************************
* LOCAL FUNCTIONS
******************************************************************************
*/
