/* $Id: xspi_stm_flash_example.c,v 1.1 2008/08/27 11:02:01 sadanan Exp $ */
/******************************************************************************
*
*       XILINX IS PROVIDING THIS DESIGN, CODE, OR INFORMATION "AS IS"
*       AS A COURTESY TO YOU, SOLELY FOR USE IN DEVELOPING PROGRAMS AND
*       SOLUTIONS FOR XILINX DEVICES.  BY PROVIDING THIS DESIGN, CODE,
*       OR INFORMATION AS ONE POSSIBLE IMPLEMENTATION OF THIS FEATURE,
*       APPLICATION OR STANDARD, XILINX IS MAKING NO REPRESENTATION
*       THAT THIS IMPLEMENTATION IS FREE FROM ANY CLAIMS OF INFRINGEMENT,
*       AND YOU ARE RESPONSIBLE FOR OBTAINING ANY RIGHTS YOU MAY REQUIRE
*       FOR YOUR IMPLEMENTATION.  XILINX EXPRESSLY DISCLAIMS ANY
*       WARRANTY WHATSOEVER WITH RESPECT TO THE ADEQUACY OF THE
*       IMPLEMENTATION, INCLUDING BUT NOT LIMITED TO ANY WARRANTIES OR
*       REPRESENTATIONS THAT THIS IMPLEMENTATION IS FREE FROM CLAIMS OF
*       INFRINGEMENT, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
*       FOR A PARTICULAR PURPOSE.
*
*       (c) Copyright 2008 Xilinx Inc.
*       All rights reserved.
*
******************************************************************************/
/******************************************************************************
*
* @file xspi_stm_flash_example.c
*
* This file contains a design example using the SPI driver (XSpi) and hardware
* device with an STM serial Flash device (M25P series) in the interrupt mode.
* This example erases a Sector, writes to a Page within the Sector, reads back
* from that Page and compares the data.
*
* This example  has been tested with an M25P16 device. The bytes per page
* (STM_PAGE_SIZE) in M25P16 is 256.
*
* This example assumes that the underlying processor is MicroBlaze.
*
* @note
*
* None.
*
* <pre>
* MODIFICATION HISTORY:
*
* Ver   Who  Date     Changes
* ----- ---- -------- -----------------------------------------------
* 1.00a sd   02/26/08 First release
*
* </pre>
*
******************************************************************************/

/***************************** Include Files *********************************/

#include "xparameters.h"	/* EDK generated parameters */
#include "xintc.h"		/* Interrupt controller device driver */
#include "xspi.h"		/* SPI device driver */
#include "mb_interface.h"	/* Microblaze interface */
#include "xuartns550_l.h"

#if !SIM
#include "stdio.h"
#endif


/************************** Constant Definitions *****************************/
#ifndef UART_BASEADDR
#define UART_BASEADDR XPAR_UARTNS550_0_BASEADDR
#endif
#define UART_CLOCK    XPAR_UARTNS550_0_CLOCK_FREQ_HZ

#if !SIM
#define UART_BAUDRATE 9600                      /* real hardware */
#else
#define UART_BAUDRATE (UART_CLOCK / 16 / 3)     /* simulation */
#endif

/*
 * The following constants map to the XPAR parameters created in the
 * xparameters.h file. They are defined here such that a user can easily
 * change all the needed parameters in one place.
 */
#define SPI_DEVICE_ID			XPAR_SPI_0_DEVICE_ID
#define INTC_DEVICE_ID			XPAR_INTC_0_DEVICE_ID
#define SPI_INTR_ID			XPAR_INTERRUPT_CNTLR_SPI_FLASH_IP2INTC_IRPT_INTR

/*
 * The following constant defines the slave select signal that is used to
 * to select the Flash device on the SPI bus, this signal is typically
 * connected to the chip select of the device.
 */
#define STM_SPI_SELECT 0x01

/*
 * Definitions of the commands shown in this example.
 */
#define STM_COMMAND_RANDOM_READ		0x03 /* Random read command */
#define STM_COMMAND_PAGEPROGRAM_WRITE	0x02 /* Page Program command */
#define	STM_COMMAND_WRITE_ENABLE	0x06 /* Write Enable command */
#define STM_COMMAND_SECTOR_ERASE	0xD8 /* Sector Erase command */
#define STM_COMMAND_BULK_ERASE		0xC7 /* Bulk Erase command */
#define STM_COMMAND_STATUSREG_READ	0x05 /* Status read command */
#define STM_COMMAND_STATUSREG_VENDOR	0x9F /* Read Vendor ID */



/**
 * This definitions specify the EXTRA bytes in each of the command
 * transactions. This count includes Command byte, address bytes and any
 * don't care bytes needed.
 */
#define STM_READ_WRITE_EXTRA_BYTES	4 /* Read/Write extra bytes */
#define	STM_WRITE_ENABLE_BYTES		1 /* Write Enable bytes */
#define STM_SECTOR_ERASE_BYTES		4 /* Sector erase extra bytes */
#define STM_BULK_ERASE_BYTES		1 /* Bulk erase extra bytes */
#define STM_STATUS_READ_BYTES		2 /* Status read bytes count */
#define STM_STATUS_WRITE_BYTES		2 /* Status write bytes count */

/*
 * Flash not busy mask in the status register of the flash device.
 */
#define STM_FLASH_SR_IS_READY_MASK	0x01 /* Ready mask */

/*
 * Number of bytes per page in the flash device.
 */
#define STM_PAGE_SIZE		256

/*
 * Address of the page to perform Erase, Write and Read operations.
 */
#define STM_FLASH_TEST_ADDRESS	0xC000

/*
 * Byte offset value written to Flash. This needs to redefined for writing
 * different patterns of data to the Flash device.
 */
#define STM_FLASH_TEST_BYTE	0x20

/*
 * Byte Positions.
 */
#define BYTE1				0 /* Byte 1 position */
#define BYTE2				1 /* Byte 2 position */
#define BYTE3				2 /* Byte 3 position */
#define BYTE4				3 /* Byte 4 position */
#define BYTE5				4 /* Byte 5 position */

#define STM_DUMMYBYTE			0xFF /* Dummy byte */

/**************************** Type Definitions *******************************/

/***************** Macros (Inline Functions) Definitions *********************/

/************************** Function Prototypes ******************************/
/*
 * Return to the bootloader via it's well-known vector at 0x40
 */
static void
return_to_loader()
{
    void (*loader_start1)();

    loader_start1 = (void*) (*(unsigned*)0x40);
    if (loader_start1) {
        (*loader_start1)();
    }
}


int SpiStmFlashWriteEnable(XSpi *SpiPtr);
int SpiStmFlashWrite(XSpi *SpiPtr, u32 Addr, u32 ByteCount);
int SpiStmFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount);
int SpiStmFlashBulkErase(XSpi *SpiPtr);
int SpiStmFlashSectorErase(XSpi *SpiPtr, u32 Addr);
int SpiStmFlashGetStatus(XSpi *SpiPtr);
int SpiStmFlashGetVendorID(XSpi *SpiPtr);
static  __inline__ int SpiStmFlashWaitForFlashNotBusy(void);
void SpiHandler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount);
static int SetupInterruptSystem(XSpi *SpiPtr);

/************************** Variable Definitions *****************************/

/*
 * The instances to support the device drivers are global such that they
 * are initialized to zero each time the program runs. They could be local
 * but should at least be static so they are zeroed.
 */
static XIntc InterruptController;
static XSpi Spi;

/*
 * The following variables are shared between non-interrupt processing and
 * interrupt processing such that they must be global.
 */
volatile static int TransferInProgress;

/*
 * The following variable tracks any errors that occur during interrupt
 * processing.
 */
int ErrorCount;

/*
 * Buffers used during read and write transactions.
 */
u8 ReadBuffer[STM_PAGE_SIZE + STM_READ_WRITE_EXTRA_BYTES];
u8 WriteBuffer[STM_PAGE_SIZE + STM_READ_WRITE_EXTRA_BYTES];

/************************** Function Definitions ******************************/

/*****************************************************************************/
/**
*
* Main function to execute the Flash example.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
#if SIM
static void printf(char *s)
{
  while (*s) {
    XUartNs550_SendByte(UART_BASEADDR, *s);
    s++;
  }
}
#endif

int main()
{
	int Status;

   XUartNs550_SetBaud(UART_BASEADDR, UART_CLOCK, UART_BAUDRATE);
   XUartNs550_SetLineControlReg(UART_BASEADDR, XUN_LCR_8_DATA_BITS);

  xil_printf("\n\r********************************************************");
  xil_printf("\n\r********************************************************");
  xil_printf("\n\r**     SP605 - SPI Flash Test                         **");
  xil_printf("\n\r********************************************************");
  xil_printf("\n\r********************************************************\r\n");

	/*
	 * Initialize the SPI driver so that it's ready to use,
	 * specify the device ID that is generated in xparameters.h.
	 */
	Status = XSpi_Initialize(&Spi, SPI_DEVICE_ID);
	if(Status != XST_SUCCESS) {\
   		xil_printf("Failure INIT\r\n");
		return XST_FAILURE;
	}


	/*
	 * Connect the SPI driver to the interrupt subsystem such that
	 * interrupts can occur. This function is application specific.
	 */
	Status = SetupInterruptSystem(&Spi);
	if(Status != XST_SUCCESS) {
   		xil_printf("Failure Interrupt\r\n");
		return XST_FAILURE;
	}

	/*
	 * Setup the handler for the SPI that will be called from the interrupt
	 * context when an SPI status occurs, specify a pointer to the SPI
	 * driver instance as the callback reference so the handler is able to
	 * access the instance data.
	 */
	XSpi_SetStatusHandler(&Spi, &Spi, (XSpi_StatusHandler)SpiHandler);

	/*
	 * Set the SPI device as a master and in manual slave select mode such
	 * that the slave select signal does not toggle for every byte of a
	 * transfer, this must be done before the slave select is set.
	 */
	Status = XSpi_SetOptions(&Spi, XSP_MASTER_OPTION |
						XSP_MANUAL_SSELECT_OPTION);
	if(Status != XST_SUCCESS) {
   		xil_printf("Failure Options\r\n");
		return XST_FAILURE;
	}

	/*
	 * Select the STM flash device on the SPI bus, so that it can be
	 * read and written using the SPI bus.
	 */
	Status = XSpi_SetSlaveSelect(&Spi, STM_SPI_SELECT);
	if(Status != XST_SUCCESS) {
   		xil_printf("Failure Slave Select\r\n");
		return XST_FAILURE;
	}

	/*
	 * Start the SPI driver so that interrupts and the device are enabled.
	 */
	XSpi_Start(&Spi);

	SpiStmFlashGetVendorID(&Spi);

   return_to_loader();

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function enables writes to the STM Serial Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiStmFlashWriteEnable(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_WRITE_ENABLE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				STM_WRITE_ENABLE_BYTES);
	if(Status != XST_SUCCESS) {
		xil_printf("Error in transfer\r\n");
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		xil_printf("The error count is %x\r\n",ErrorCount);
		ErrorCount = 0;
		xil_printf("Error in tranfer stream\r\n");
		return XST_FAILURE;
	}


	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function writes the data to the specified locations in the STM Serial
* Flash memory.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address in the Buffer, where to write the data.
* @param	ByteCount is the number of bytes to be written.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiStmFlashWrite(XSpi *SpiPtr, u32 Addr, u32 ByteCount)
{
	u32 Index;
	int Status;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_PAGEPROGRAM_WRITE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;


	/*
	 * Fill in the TEST data that is to be written into the STM Serial Flash
	 * device.
	 */
	for(Index = 4; Index < ByteCount + STM_READ_WRITE_EXTRA_BYTES;
								Index++) {
		WriteBuffer[Index] = (u8)((Index - 4) + STM_FLASH_TEST_BYTE);
	}

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
				(ByteCount + STM_READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the data from the STM Serial Flash Memory
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the starting address in the Flash Memory from which the
*		data is to be read.
* @param	ByteCount is the number of bytes to be read.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		None
*
******************************************************************************/
int SpiStmFlashRead(XSpi *SpiPtr, u32 Addr, u32 ByteCount)
{
	int Status;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_RANDOM_READ;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) Addr;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer( SpiPtr, WriteBuffer, ReadBuffer,
				(ByteCount + STM_READ_WRITE_EXTRA_BYTES));
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction.
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the entire contents of the STM Serial Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will read as 0xFF.
*
******************************************************************************/
int SpiStmFlashBulkErase(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_BULK_ERASE;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
						STM_BULK_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function erases the contents of the specified Sector in the STM Serial
* Flash device.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
* @param	Addr is the address within a sector of the Buffer, which is to
*		be erased.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The erased bytes will be read back as 0xFF.
*
******************************************************************************/
int SpiStmFlashSectorErase(XSpi *SpiPtr, u32 Addr)
{
	int Status;

	/*
	 * Prepare the WriteBuffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_SECTOR_ERASE;
	WriteBuffer[BYTE2] = (u8) (Addr >> 16);
	WriteBuffer[BYTE3] = (u8) (Addr >> 8);
	WriteBuffer[BYTE4] = (u8) (Addr);

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, NULL,
					STM_SECTOR_ERASE_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function reads the Status register of the STM Flash.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		The status register content is stored at the second byte pointed
*		by the ReadBuffer.
*
******************************************************************************/
int SpiStmFlashGetStatus(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_STATUSREG_READ;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						STM_STATUS_READ_BYTES);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction..
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	return XST_SUCCESS;
}

int SpiStmFlashGetVendorID(XSpi *SpiPtr)
{
	int Status;

	/*
	 * Prepare the Write Buffer.
	 */
	WriteBuffer[BYTE1] = STM_COMMAND_STATUSREG_VENDOR;

	/*
	 * Initiate the Transfer.
	 */
	TransferInProgress = TRUE;
	Status = XSpi_Transfer(SpiPtr, WriteBuffer, ReadBuffer,
						4);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Wait till the Transfer is complete and check if there are any errors
	 * in the transaction.
	 */
	while(TransferInProgress);
	if(ErrorCount != 0) {
		ErrorCount = 0;
		return XST_FAILURE;
	}

	xil_printf("Manufacturer ID: ");
	xil_printf("%x\r\n",ReadBuffer[1]);
	xil_printf("Device ID: ");
	xil_printf("%x",ReadBuffer[2]);
	xil_printf("%x\r\n",ReadBuffer[3]);

	return XST_SUCCESS;
}


/*****************************************************************************/
/**
*
* This function waits till the STM serial Flash is ready to accept next command.
*
* @param	None
*
* @return	XST_SUCCESS if successful else XST_FAILURE.
*
* @note		This function reads the status register of the Buffer and waits
*.		till the WIP bit of the status register becomes 0.
*
******************************************************************************/
int SpiStmFlashWaitForFlashNotBusy(void)
{
	int Status;
	u8 StatusReg;


	while(1) {

		/*
		 * Get the Status Register.
		 */
		Status = SpiStmFlashGetStatus(&Spi);
		if(Status != XST_SUCCESS) {
			return XST_FAILURE;
		}

		/*
		 * Check if the flash is ready to accept the next command.
		 * If so break.
		 */
		StatusReg = ReadBuffer[1];
		if((StatusReg & STM_FLASH_SR_IS_READY_MASK) == 0) {
			break;
		}
	}

	return XST_SUCCESS;
}

/*****************************************************************************/
/**
*
* This function is the handler which performs processing for the SPI driver.
* It is called from an interrupt context such that the amount of processing
* performed should be minimized. It is called when a transfer of SPI data
* completes or an error occurs.
*
* This handler provides an example of how to handle SPI interrupts and
* is application specific.
*
* @param	CallBackRef is the upper layer callback reference passed back
*		when the callback function is invoked.
* @param	StatusEvent is the event that just occurred.
* @param	ByteCount is the number of bytes transferred up until the event
*		occurred.
*
* @return	None.
*
* @note		None.
*
******************************************************************************/
void SpiHandler(void *CallBackRef, u32 StatusEvent, unsigned int ByteCount)
{
	/*
	 * Indicate the transfer on the SPI bus is no longer in progress
	 * regardless of the status event.
	 */
	TransferInProgress = FALSE;

	/*
	 * If the event was not transfer done, then track it as an error.
	 */
	if (StatusEvent != XST_SPI_TRANSFER_DONE) {
		xil_printf("The status is %x\r\n",StatusEvent);
		ErrorCount++;
	}
}

/*****************************************************************************/
/**
*
* This function setups the interrupt system such that interrupts can occur
* for the Spi device. This function is application specific since the actual
* system may or may not have an interrupt controller. The Spi device could be
* directly connected to a processor without an interrupt controller.  The
* user should modify this function to fit the application.
*
* @param	SpiPtr is a pointer to the instance of the Spi device.
*
* @return	XST_SUCCESS if successful, otherwise XST_FAILURE.
*
* @note		None
*
******************************************************************************/
static int SetupInterruptSystem(XSpi *SpiPtr)
{

	int Status;

	/*
	 * Initialize the interrupt controller driver so that
	 * it's ready to use, specify the device ID that is generated in
	 * xparameters.h
	 */
	Status = XIntc_Initialize(&InterruptController, INTC_DEVICE_ID);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Connect a device driver handler that will be called when an interrupt
	 * for the device occurs, the device driver handler performs the
	 * specific interrupt processing for the device
	 */
	Status = XIntc_Connect(&InterruptController,
				SPI_INTR_ID,
				(XInterruptHandler)XSpi_InterruptHandler,
				(void *)SpiPtr);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Start the interrupt controller such that interrupts are enabled for
	 * all devices that cause interrupts, specific real mode so that
	 * the SPI can cause interrupts thru the interrupt controller.
	 */
	Status = XIntc_Start(&InterruptController, XIN_REAL_MODE);
	if(Status != XST_SUCCESS) {
		return XST_FAILURE;
	}

	/*
	 * Enable the interrupt for the SPI.
	 */
	XIntc_Enable(&InterruptController, SPI_INTR_ID);

	/*
	 * Enable the MicroBlaze Interrupts.
	 */
	microblaze_enable_interrupts();

	return XST_SUCCESS;
}

