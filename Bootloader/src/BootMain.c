/**
 * @file      BootMain.c
 * @brief     Main file for the ESE5160 bootloader. Handles updating the main application
 * @details   Main file for the ESE5160 bootloader. Handles updating the main application
 * @author    Eduardo Garcia
 * @author    Nick M-G
 * @date      2024-03-03
 * @version   2.0
 * @copyright Copyright University of Pennsylvania
 ******************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "conf_example.h"
#include "sd_mmc_spi.h"
#include <asf.h>
#include <string.h>

#include "ASF/sam0/drivers/dsu/crc32/crc32.h"
#include "SD Card/SdCard.h"
#include "SerialConsole/SerialConsole.h"
#include "Systick/Systick.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define APP_START_ADDRESS           ((uint32_t) 0x12000)                    ///< Start of main application. Must be address of start of main application
#define APP_START_RESET_VEC_ADDRESS (APP_START_ADDRESS + (uint32_t) 0x04)   ///< Main application reset vector address
#define CRC32_TESTA	0x1EF640AF
#define CRC32_TESTB	0xFFECCCF0

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/

struct usart_module cdc_uart_module;   ///< Structure for UART module connected to EDBG (used for unit test output)

/******************************************************************************
 * Local Function Declaration
 ******************************************************************************/
static void jumpToApplication(void);
static bool StartFilesystemAndTest(void);
static void configure_nvm(void);

/******************************************************************************
 * Global Variables
 ******************************************************************************/
// INITIALIZE VARIABLES
char test_file_name[] = "0:sd_mmc_test.txt";   ///< Test TEXT File name
char test_bin_file[] = "0:sd_binary.bin";      ///< Test BINARY File name
Ctrl_status status;                            ///< Holds the status of a system initialization
FRESULT res;                                   // Holds the result of the FATFS functions done on the SD CARD TEST
FATFS fs;                                      // Holds the File System of the SD CARD
FIL file_object;                               // FILE OBJECT used on main for the SD Card Test

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
* @fn		int main(void)
* @brief	Main function for ESE5160 Bootloader Application

* @return	Unused (ANSI-C compatibility).
* @note		Bootloader code initiates here.
*****************************************************************************/

int main(void) {
    /*1.) INIT SYSTEM PERIPHERALS INITIALIZATION*/
	
    system_init();
    delay_init();
    InitializeSerialConsole();
    system_interrupt_enable_global();

    /* Initialize SD MMC stack */
    sd_mmc_init();

    // Initialize the NVM driver
    configure_nvm();

    irq_initialize_vectors();
    cpu_irq_enable();

    // Configure CRC32
    dsu_crc32_init();

    SerialConsoleWriteString("ESE5160 - ENTER BOOTLOADER");   // Order to add string to TX Buffer

    /*END SYSTEM PERIPHERALS INITIALIZATION*/

    /*2.) STARTS SIMPLE SD CARD MOUNTING AND TEST!*/
	
    // EXAMPLE CODE ON MOUNTING THE SD CARD AND WRITING TO A FILE
    // See function inside to see how to open a file
    SerialConsoleWriteString("\x0C\n\r-- SD/MMC Card Example on FatFs --\n\r");

    if (StartFilesystemAndTest() == false) {
        SerialConsoleWriteString("SD CARD failed! Check your connections. System will restart in 5 seconds...");
        delay_cycles_ms(5000);
        system_reset();
    } else {
        SerialConsoleWriteString("SD CARD mount success! Filesystem also mounted. \r\n");
    }

    /*END SIMPLE SD CARD MOUNTING AND TEST!*/

    /*3.) STARTS BOOTLOADER HERE!*/
	
	// Check Flag
	char flag_to_check[] = "0:FlagA.txt";  // flag that will be checked
	char firmware_to_flash[32] = {0};  // the firmware that will be flashed into MCU
	FILINFO flag_info;  // flag information returned from f_stat
	
	FRESULT f_stat_res = f_stat(flag_to_check, &flag_info);  // check if FlagA is there or not
	if (f_stat_res == FR_OK)
	{
		strcpy(firmware_to_flash, "0:TestA.bin");
		SerialConsoleWriteString("Found FlagA.txt, preparing to flash TestA.bin...\r\n");
	}
	else {
		strcpy(flag_to_check, "0:FlagB.txt");
		f_stat_res = f_stat(flag_to_check, &flag_info);
		if (f_stat_res == FR_OK)
		{
			strcpy(firmware_to_flash, "0:TestB.bin");
			SerialConsoleWriteString("Found FlagB.txt, preparing to flash TestB.bin...\r\n");
		}
	}
	
	// Update MCU FW with SD card FW
	FIL firmware_file;  // firmware file that will be opened
		
	if (firmware_to_flash != '\0')
	{
		// Open the corresponding file
		FRESULT f_open_res = f_open(&firmware_file, (char const*)firmware_to_flash, FA_READ);
		if (f_open_res != FR_OK)
		{
			SerialConsoleWriteString("ERROR: Failed to open firmware binary file.\r\n");
			system_reset();
		}
		
		DWORD firmware_size = f_size(&firmware_file);  // get the firmware size

		SerialConsoleWriteString("Flashing firmware...\r\n");  // ready to flash firmware
		
		// Erase firmware in MCU
		uint32_t i_des = firmware_size % NVMCTRL_ROW_SIZE == 0 ? (firmware_size / NVMCTRL_ROW_SIZE) : (firmware_size / NVMCTRL_ROW_SIZE + 1);
		for (uint32_t i = 0; i < i_des; i++)
		{
			nvm_erase_row(APP_START_ADDRESS + i * NVMCTRL_ROW_SIZE);
		}
		
		// Write to flash
		UINT bytes_read;  // number of bytes read
		uint8_t page_buffer[NVMCTRL_PAGE_SIZE];  // flash page size
		
		uint32_t app_write_address = APP_START_ADDRESS;
		while(1) {
			memset(page_buffer, 0xFF, NVMCTRL_PAGE_SIZE);
			res = f_read(&firmware_file, page_buffer, NVMCTRL_PAGE_SIZE, &bytes_read);
			if (res != FR_OK || bytes_read == 0)
			{
				break;
			}
			
			nvm_write_buffer(app_write_address, page_buffer, NVMCTRL_PAGE_SIZE);
			app_write_address += NVMCTRL_PAGE_SIZE;
		}
		
		f_close(&firmware_file);  // close the file
		
		SerialConsoleWriteString("Firmware update complete.\r\n"); 
		
		// CRC check to make sure FW update successful
		uint32_t crc = 0xFFFFFFFF;
		if (dsu_crc32_cal(APP_START_ADDRESS, firmware_size, &crc) != STATUS_OK)
		{
			system_reset();
		}
		else {
			if (strcmp(firmware_to_flash, "0:TestA.bin") == 0 && (~crc) == CRC32_TESTA)
			{
				f_unlink(flag_to_check);  // delete the flag after update firmware
			}
			else if (strcmp(firmware_to_flash, "0:TestB.bin") == 0 && (~crc) == CRC32_TESTB)
			{
				f_unlink(flag_to_check);  // delete the flag after update firmware
			}
		}
	}
	else {
		SerialConsoleWriteString("No update flag found. Skipping firmware update.\r\n");
	}

    /* END BOOTLOADER HERE!*/

    // 4.) DEINITIALIZE HW AND JUMP TO MAIN APPLICATION!
    SerialConsoleWriteString("ESE5160 - EXIT BOOTLOADER");   // Order to add string to TX Buffer
    delay_cycles_ms(100);                                    // Delay to allow print

    // Deinitialize HW - deinitialize started HW here!
    DeinitializeSerialConsole();   // Deinitializes UART
    sd_mmc_deinit();               // Deinitialize SD CARD

    // Jump to application
    jumpToApplication();

    // Should not reach here! The device should have jumped to the main FW.
}

/******************************************************************************
 * Static Functions
 ******************************************************************************/

/**
 * function      static void StartFilesystemAndTest()
 * @brief        Starts the filesystem and tests it. Sets the filesystem to the global variable fs
 * @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
 *				before performing the jump!
 * @return       Returns true is SD card and file system test passed. False otherwise.
 ******************************************************************************/
static bool StartFilesystemAndTest(void) {
    bool sdCardPass = true;
    uint8_t binbuff[256];

    // Before we begin - fill buffer for binary write test
    // Fill binbuff with values 0x00 - 0xFF
    for (int i = 0; i < 256; i++) {
        binbuff[i] = i;
    }

    // MOUNT SD CARD
    Ctrl_status sdStatus = SdCard_Initiate();
    if (sdStatus == CTRL_GOOD)   // If the SD card is good we continue mounting the system!
    {
        SerialConsoleWriteString("SD Card initiated correctly!\n\r");

        // Attempt to mount a FAT file system on the SD Card using FATFS
        SerialConsoleWriteString("Mount disk (f_mount)...\r\n");
        memset(&fs, 0, sizeof(FATFS));
        res = f_mount(LUN_ID_SD_MMC_0_MEM, &fs);   // Order FATFS Mount		
        if (FR_INVALID_DRIVE == res) {
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }
        SerialConsoleWriteString("[OK]\r\n");

        // Create and open a file
        SerialConsoleWriteString("Create a file (f_open)...\r\n");

        test_file_name[0] = LUN_ID_SD_MMC_0_MEM + '0';
        res = f_open(&file_object, (char const *) test_file_name, FA_CREATE_ALWAYS | FA_WRITE);

        if (res != FR_OK) {
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");

        // Write to a file
        SerialConsoleWriteString("Write to test file (f_puts)...\r\n");

        if (0 == f_puts("Test SD/MMC stack\n", &file_object)) {
            f_close(&file_object);
            LogMessage(LOG_INFO_LVL, "[FAIL]\r\n");
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");
        f_close(&file_object);   // Close file
        SerialConsoleWriteString("Test is successful.\n\r");

        // Write binary file
        // Read SD Card File
        test_bin_file[0] = LUN_ID_SD_MMC_0_MEM + '0';
        res = f_open(&file_object, (char const *) test_bin_file, FA_WRITE | FA_CREATE_ALWAYS);

        if (res != FR_OK) {
            SerialConsoleWriteString("Could not open binary file!\r\n");
            LogMessage(LOG_INFO_LVL, "[FAIL] res %d\r\n", res);
            sdCardPass = false;
            goto main_end_of_test;
        }

        // Write to a binaryfile
        SerialConsoleWriteString("Write to test file (f_write)...\r\n");
        uint32_t varWrite = 0;
        if (0 != f_write(&file_object, binbuff, 256, &varWrite)) {
            f_close(&file_object);
            LogMessage(LOG_INFO_LVL, "[FAIL]\r\n");
            sdCardPass = false;
            goto main_end_of_test;
        }

        SerialConsoleWriteString("[OK]\r\n");
        f_close(&file_object);   // Close file
        SerialConsoleWriteString("Test is successful.\n\r");

    main_end_of_test:
        SerialConsoleWriteString("End of Test.\n\r");

    } else {
        SerialConsoleWriteString("SD Card failed initiation! Check connections!\n\r");
        sdCardPass = false;
    }

    return sdCardPass;
}

/**
 * function      static void jumpToApplication(void)
 * @brief        Jumps to main application
 * @details      Jumps to the main application. Please turn off ALL PERIPHERALS that were turned on by the bootloader
 *				before performing the jump!
 * @return
 ******************************************************************************/
static void jumpToApplication(void) {
    // Function pointer to application section
    void (*applicationCodeEntry)(void);

    // Rebase stack pointer
    __set_MSP(*(uint32_t *) APP_START_ADDRESS);

    // Rebase vector table
    SCB->VTOR = ((uint32_t) APP_START_ADDRESS & SCB_VTOR_TBLOFF_Msk);

    // Set pointer to application section
    applicationCodeEntry = (void (*)(void))(unsigned *) (*(unsigned *) (APP_START_RESET_VEC_ADDRESS));

    // Jump to application. By calling applicationCodeEntry() as a function we move the PC to the point in memory pointed by applicationCodeEntry,
    // which should be the start of the main FW.
    applicationCodeEntry();
}

/**
 * function      static void configure_nvm(void)
 * @brief        Configures the NVM driver
 * @details
 * @return
 ******************************************************************************/
static void configure_nvm(void) {
    struct nvm_config config_nvm;
    nvm_get_config_defaults(&config_nvm);
    config_nvm.manual_page_write = false;
    nvm_set_config(&config_nvm);
}
