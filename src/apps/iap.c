/*
 * cryption_process.c
 *
 *  Created on: Feb 28, 2016
 *      Author: tgjuranec
 */
#include <board.h>
#include <string.h>
#include <cr_section_macros.h>



uint8_t flash_buff[256] __attribute__((aligned(16)));



#define START_ADDR_LAST_SECTOR  0x00078000
/* Size of each sector */
#define SECTOR_SIZE             1024
/* LAST SECTOR */
#define IAP_LAST_SECTOR         29
/* Number of bytes to be written to the last sector */
#define IAP_NUM_BYTES_TO_WRITE  256


void get_flash(){
	uint32_t flash_key_address = START_ADDR_LAST_SECTOR;
	memcpy(flash_buff,(uint32_t *)flash_key_address,256);
}


int write_flash(void)
{
	uint8_t ret_code;
	/* Read Part Identification Number*/
	Chip_IAP_ReadPID();
	/* Disable interrupt mode so it doesn't fire during FLASH updates */
	__disable_irq();
	/* IAP Flash programming */
	/* Prepare to write/erase the last sector */
	ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);
	/* Erase the last sector */
	ret_code = Chip_IAP_EraseSector(IAP_LAST_SECTOR, IAP_LAST_SECTOR);
	/* Prepare to write/erase the last sector */
	ret_code = Chip_IAP_PreSectorForReadWrite(IAP_LAST_SECTOR, IAP_LAST_SECTOR);
	/* Write to the last sector */
	ret_code = Chip_IAP_CopyRamToFlash(START_ADDR_LAST_SECTOR,(uint32_t *) flash_buff, 256);
	/* Re-enable interrupt mode */
	__enable_irq();
	/* Start the signature generator for the last sector */
	Chip_FMC_ComputeSignatureBlocks(START_ADDR_LAST_SECTOR, (SECTOR_SIZE / 16));
	/* Check for signature geenration completion */
	while (Chip_FMC_IsSignatureBusy()) {}
	return ret_code;
}

