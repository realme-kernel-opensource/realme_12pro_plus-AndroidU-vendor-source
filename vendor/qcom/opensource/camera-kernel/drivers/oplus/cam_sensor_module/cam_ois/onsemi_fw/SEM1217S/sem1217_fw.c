/////////////////////////////////////////////////////////////////////////////
// File Name    : bu24721_fw.c
// Function        : Various function for OIS control
//
// Copyright(c)    Rohm Co.,Ltd. All rights reserved
//
/***** ROHM Confidential ***************************************************/
//#define    _USE_MATH_DEFINES                            //

#ifndef SEM1217_FW_C
#define SEM1217_FW_C
#endif
#include <linux/types.h>
#include "cam_sensor_util.h"
#include "cam_debug_util.h"
#include "fw_download_interface.h"
#include "sem1217_fw.h"
#include "firmware.h"

static int block_write_count = 0;
void SEM1217S_Store_OIS_Cal_Data (void)
{
	uint8_t txdata[SEM1217S_TX_BUFFER_SIZE];
	uint8_t rxdata[SEM1217S_RX_BUFFER_SIZE];
	uint16_t repeatedCnt = 1000;
	uint16_t data_error;

	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_OIS_STS);
	CAM_ERR(CAM_OIS, "SEM1217S_REG_OIS_STS: %u", rxdata[0]);

	if (rxdata[0] != SEM1217S_STATE_READY)
	{
		txdata[0] = SEM1217S_OIS_OFF;
		sem1217_8bit_write(SEM1217S_REG_OIS_CTRL, txdata[0]);
	}

	txdata[0] = SEM1217S_OIS_INFO_EN; /* Set OIS_INFO_EN */
	/* Write 1 Byte to REG_INFO_BLK_UP_CTRL */
	sem1217_8bit_write(SEM1217S_REGINFO_BLK_UP_CTRL, txdata[0]);
	CAM_ERR(CAM_OIS, "write SEM1217S_REGINFO_BLK_UP_CTRL: %u", txdata[0]);
	//I2C_Write_Data(REG_INFO_BLK_UP_CTRL, 1, txdata);
	mdelay(100); /* Delay 100 ms */

	do
	{
		if (repeatedCnt == 0)
		{
			/* Abnormal Termination Error. */
			CAM_ERR(CAM_OIS, "REG_INFO_BLK_UP_CTRL failed: %u", rxdata[0]);
			return;
		}
		mdelay(50); /* Delay 50 ms */
		rxdata[0]=sem1217_8bit_read(SEM1217S_REGINFO_BLK_UP_CTRL);
		repeatedCnt--;
	} while ((rxdata[0] & SEM1217S_OIS_INFO_EN) == SEM1217S_OIS_INFO_EN);

	data_error=sem1217_16bit_read(SEM1217S_REG_OIS_ERR); /* Read REG_OIS_ERR */

	if ((data_error & SEM1217S_ERR_ODI) != SEM1217S_NO_ERROR)
	{
		/* Different INFORWRITE data on flash */
		CAM_ERR(CAM_OIS, "SEM1217S_ERR_ODI error %d", data_error);
		return;
	}
	/* INFORWRITE data on flash Success Process */
}

struct SEM1217S_FACT_ADJ SEM1217S_Gyro_offset_cal(void)
{
	uint8_t txdata[SEM1217S_TX_BUFFER_SIZE];
	uint8_t rxdata[SEM1217S_RX_BUFFER_SIZE];
	uint16_t repeatedCnt = 1000;
	uint16_t data_error;
	struct SEM1217S_FACT_ADJ SEM1217S_FADJ_CAL = { 0x0000, 0x0000};

	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_OIS_STS);
	CAM_ERR(CAM_OIS, "SEM1217S_REG_OIS_STS: %u", rxdata[0]);

	if (rxdata[0] != SEM1217S_STATE_READY)
	{
		txdata[0] = SEM1217S_OIS_OFF;
		sem1217_8bit_write(SEM1217S_REG_OIS_CTRL, txdata[0]);
	}

	txdata[0] = SEM1217S_G_OFFSET_EN;
	sem1217_8bit_write(SEM1217S_REG_GCAL_CTRL, txdata[0]);
	mdelay(50);

	do
	{
		if (repeatedCnt == 0)
		{
			return SEM1217S_FADJ_CAL;
		}
		mdelay(50);
		rxdata[0] = sem1217_8bit_read(SEM1217S_REG_GCAL_CTRL);
        CAM_ERR(CAM_OIS, "SEM1217S_REG_GCAL_CTRL: %u", rxdata[0]);
		repeatedCnt--;
	} while ((rxdata[0] & SEM1217S_G_OFFSET_EN) == SEM1217S_G_OFFSET_EN);

	/*rxdata[0] = sem1217_8bit_read(SEM1217S_REG_OIS_ERR);
	rxdata[1] = sem1217_8bit_read(SEM1217S_REG_OIS_ERR + 1);
	data_error = ((uint16_t*)rxdata)[0];*/

	data_error = sem1217_16bit_read(SEM1217S_REG_OIS_ERR);
	if ((data_error & (SEM1217S_ERR_GCALX | SEM1217S_ERR_GCALY)) != SEM1217S_NO_ERROR)
	{
		CAM_ERR(CAM_OIS, "SEM1217S offset cal failed, data_error: 0x%x", data_error);
	}

	SEM1217S_FADJ_CAL.gl_GX_OFS = sem1217_16bit_read(SEM1217S_REG_GX_OFFSET);
	SEM1217S_FADJ_CAL.gl_GY_OFS = sem1217_16bit_read(SEM1217S_REG_GY_OFFSET);
	CAM_ERR(CAM_OIS, "czq SEM1217S x-offset: %u, y-offset: %u", SEM1217S_FADJ_CAL.gl_GX_OFS, SEM1217S_FADJ_CAL.gl_GY_OFS);
	SEM1217S_Store_OIS_Cal_Data();

	return SEM1217S_FADJ_CAL;
}

void GyroWrite(uint32_t address, uint32_t gain)
{
	unsigned char txdata[4];
	txdata[0] = gain & 0x00FF;
	txdata[1] = (gain & 0xFF00) >> 8;
	txdata[2] = (gain & 0xFF0000) >> 16;
	txdata[3] = (gain & 0xFF000000) >> 24;
	sem1217_8bit_write(address, txdata[0]); /* write REG_GGX Little endian*/
	sem1217_8bit_write(address+1, txdata[1]);
	sem1217_8bit_write(address+2, txdata[2]);
	sem1217_8bit_write(address+3, txdata[3]);
	CAM_INFO(CAM_OIS, "[GyroRead] gain = %u, address= 0x%x, write = 0x%x %x %x %x", gain, address, txdata[0], txdata[1], txdata[2], txdata[3]);
}

void GyroRead(uint32_t address)
{
	unsigned char rxdata[4];
	rxdata[0] = sem1217_8bit_read(address);
	rxdata[1] = sem1217_8bit_read(address+1);
	rxdata[2] = sem1217_8bit_read(address+2);
	rxdata[3] = sem1217_8bit_read(address+3);
	CAM_INFO(CAM_OIS, "[GyroRead] address= 0x%x, read = 0x%x %x %x %x", address, rxdata[0], rxdata[1], rxdata[2], rxdata[3]);
}


void SEM1217S_Gyro_gain_set(uint32_t X_gain, uint32_t Y_gain)
{
	uint8_t rxdata[SEM1217S_RX_BUFFER_SIZE];
	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_OIS_STS);
	CAM_INFO(CAM_OIS, "SEM1217S_Gyro_gain_set SEM1217S_REG_OIS_STS: %u", rxdata[0]);

	if (rxdata[0] == SEM1217S_STATE_READY)
	{
		sem1217_8bit_write(SEM1217S_GYRO_MOUNT_TYPE, SEM1217S_GYRO_MOUNT_TYPE_VALUE);
		CAM_INFO(CAM_OIS, "SEM1217S_Gyro_gain_set SEM1217S_GYRO_MOUNT_TYPE");
		/* Set Target Mode to Still Mode */
		sem1217_8bit_write(SEM1217S_REG_OIS_MODE, SEM1217S_STILL_MODE); /* Write 1 Byte to SEM1217S_REG_OIS_MODE */
		/* Start Lens Control */
		sem1217_8bit_write(SEM1217S_REG_OIS_CTRL, SEM1217S_OIS_ON); /* Write 1 Byte to REG_OIS_CTRL */
	}

	GyroRead(SEM1217S_REG_GX_GAIN); /* Read gyro gain x */
	GyroRead(SEM1217S_REG_GY_GAIN); /* Read gyro gain y */

}


int sem1217s_fw_download(struct cam_ois_ctrl_t *o_ctrl)
{
	uint8_t txdata[SEM1217S_TX_BUFFER_SIZE + 2];
	uint8_t rxdata[SEM1217S_RX_BUFFER_SIZE];
	uint8_t* chkBuff = NULL;
	uint16_t txBuffSize;
	uint32_t i, chkIdx;
	uint16_t subaddr_FLASH_DATA_BIN_1;

	uint16_t idx = 0;
	uint16_t check_sum;
	uint32_t updated_ver;
	uint32_t new_fw_ver;
	uint32_t current_fw_ver;
	int rc = 0;

	block_write_count = 0;
	chkBuff = (uint8_t*)kzalloc(SEM1217S_APP_FW_SIZE, GFP_KERNEL);

	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_APP_VER);
	rxdata[1] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 1);
	rxdata[2] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 2);
	rxdata[3] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 3);
	new_fw_ver = *(uint32_t *)&sem1217s_fw_1[SEM1217S_APP_FW_SIZE - 12];
	current_fw_ver = ((uint32_t *)rxdata)[0];

	CAM_ERR(CAM_OIS, "current_fw_ver: 0x%x, new_fw_ver: 0x%x", current_fw_ver, new_fw_ver);
	if (current_fw_ver == new_fw_ver)
	{
		CAM_ERR(CAM_OIS, "version is the same, no need to update");
		return 0;
	}

	if (current_fw_ver != 0)
	{
		rxdata[0] = sem1217_8bit_read(SEM1217S_REG_OIS_STS);
		if (rxdata[0] != SEM1217S_STATE_READY)
		{
			txdata[0] = SEM1217S_OIS_OFF;
			rc = sem1217_8bit_write(SEM1217S_REG_OIS_CTRL, txdata[0]);
			if (rc != 0)
			{
				goto error_hand;
			}
		}
		rxdata[0] = sem1217_8bit_read(SEM1217S_REG_AF_STS);
		if (rxdata[0] != SEM1217S_STATE_READY)
		{
			txdata[0] = SEM1217S_AF_OFF;
			rc = sem1217_8bit_write(SEM1217S_REG_AF_CTRL, txdata[0]);
			if (rc != 0)
			{
				goto error_hand;
			}
		}
	}

	txBuffSize = SEM1217S_TX_SIZE_256_BYTE;
	switch (txBuffSize)
	{
/*
		case SEM1217S_TX_SIZE_32_BYTE:
			txdata[0] =  SEM1217S_FWUP_CTRL_32_SET;
			break;
		case SEM1217S_TX_SIZE_64_BYTE:
			txdata[0] = SEM1217S_FWUP_CTRL_64_SET;
			break;
		case SEM1217S_TX_SIZE_128_BYTE:
			txdata[0] = SEM1217S_FWUP_CTRL_128_SET;
			break;
*/
		case SEM1217S_TX_SIZE_256_BYTE:
			txdata[0] = SEM1217S_FWUP_CTRL_256_SET;
			break;
/*
		default:
			break;
*/
	}
	rc = sem1217_8bit_write(SEM1217S_REG_FWUP_CTRL, txdata[0]);
	if (rc != 0)
	{
		goto error_hand;
	}
	msleep(60);
	check_sum = 0;

	subaddr_FLASH_DATA_BIN_1 = SEM1217S_REG_DATA_BUF;
	for (i = 0; i < (SEM1217S_APP_FW_SIZE / txBuffSize); i++)
	{
		memcpy(&chkBuff[txBuffSize * i], &sem1217s_fw_1[idx], txBuffSize);
		for (chkIdx = 0; chkIdx < txBuffSize; chkIdx += 2)
		{
			check_sum += ((chkBuff[chkIdx + 1 + (txBuffSize * i)] << 8) |  chkBuff[chkIdx + (txBuffSize * i)]);
		}
		memcpy(txdata + 2, &sem1217s_fw_1[idx], txBuffSize);
		txdata[0] = (subaddr_FLASH_DATA_BIN_1 >> 8);
		txdata[1] = (subaddr_FLASH_DATA_BIN_1 & 0xFF);
		rc = sem1217_block_write(txdata, SEM1217S_TX_BUFFER_SIZE + 2);
		if (rc != 0)
		{
			goto error_hand;
		}
		CAM_ERR(CAM_OIS, "update ois fw blk_num: %d", i+1);
		idx += txBuffSize;
		mdelay(20);
	}

	((uint16_t*)txdata)[1] = check_sum;
	txdata[0] = (SEM1217S_REG_FWUP_CHKSUM >> 8);
	txdata[1] = (SEM1217S_REG_FWUP_CHKSUM & 0xFF);
	rc = sem1217_block_write(txdata, 4);
	if (rc != 0)
	{
		goto error_hand;
	}
	mdelay(200);

	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_FWUP_ERR);
	if (rxdata[0] != SEM1217S_NO_ERROR)
	{
		CAM_ERR(CAM_OIS, "update fw erro");
		return -1;
	}

	txdata[0] = SEM1217S_RESET_REQ;
	rc = sem1217_8bit_write(SEM1217S_REG_FWUP_CTRL,txdata[0]);
	if (rc != 0)
	{
		goto error_hand;
	}
	msleep(200);

	rxdata[0] = sem1217_8bit_read(SEM1217S_REG_APP_VER);
	rxdata[1] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 1);
	rxdata[2] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 2);
	rxdata[3] = sem1217_8bit_read(SEM1217S_REG_APP_VER + 3);
	updated_ver = *(uint32_t *)rxdata;

	CAM_ERR(CAM_OIS, "updated_ver: 0x%x, new_fw_ver: 0x%x", updated_ver, new_fw_ver);
	if (updated_ver != new_fw_ver)
	{
		CAM_ERR(CAM_OIS, "update fw failed, update version is not equal with read");
		return -1;
	}
error_hand:
	CAM_ERR(CAM_OIS, "update fw failed, rc: %d", rc);

	return rc;
}

