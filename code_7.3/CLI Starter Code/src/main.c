/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * This is a bare minimum user application template.
 *
 * For documentation of the board, go \ref group_common_boards "here" for a link
 * to the board-specific documentation.
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# Basic usage of on-board LED and button
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
/*
 * Support and FAQ: visit <a href="https://www.microchip.com/support/">Microchip Support</a>
 */
#include <asf.h>
#include "SerialConsole/SerialConsole.h"
#include "CliThread.h"
/******************************************************************************
 * Includes
 ******************************************************************************/

/******************************************************************************
 * Defines and Types
 ******************************************************************************/

/******************************************************************************
 * Local Function Declaration
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/
static char bufferPrint[64];			  ///< Buffer for daemon task
static TaskHandle_t cliTaskHandle = NULL; //!< CLI task handle

#define MAX_RX_BUFFER_LENGTH 5
volatile uint8_t rx_buffer[MAX_RX_BUFFER_LENGTH];

volatile char buffer[7];

int main(void)
{
	// Board Initialization -- Code that initializes the HW and happens only once
	system_init();
	InitializeSerialConsole();

	/* Insert application code here, after the board has been initialized. */

	system_interrupt_enable_global();

	 SerialConsoleWriteString("ESE5160 - CLI and Debug Logger\r\n"); // Order to add string to TX Buffer

	 char string[] = "CLI starter code - ESE516\r\n";

	 /*Simple DebugLogger Test*/
	 setLogLevel(LOG_INFO_LVL);
	 LogMessage(LOG_INFO_LVL, "%s", string);									  // Test
	 setLogLevel(LOG_ERROR_LVL);												  // Sets the Debug Logger to only allow messages with LOG_ERROR_LVL or higher to be printed
	 LogMessage(LOG_INFO_LVL, "Performing Temperature Test...\r\n");			  // This should NOT print
	 LogMessage(LOG_FATAL_LVL, "Error! Temperature over %d Degrees!\r\n", 55); // This should print

	 LogMessage(LOG_INFO_LVL, "ESE5160 CLI STARTER PROJECT STARTED\r\n");

	// Start FreeRTOS scheduler.
	vTaskStartScheduler();

	while (1)
		;
}

/**************************************************************************/ 
/**
 * function          StartTasks
 * @brief            Initialize application tasks in this function
 * @details
 * @param[in]        None
 * @return           None
 *****************************************************************************/
static void StartTasks(void)
{

	snprintf(bufferPrint, 64, "Heap before starting tasks: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);

	// CODE HERE: Initialize any Tasks in your system here

	if (xTaskCreate(vCommandConsoleTask, "CLI_TASK", CLI_TASK_SIZE, NULL, CLI_PRIORITY, &cliTaskHandle) != pdPASS)
	{
		SerialConsoleWriteString("ERR: CLI task could not be initialized!\r\n");
	}

	snprintf(bufferPrint, 64, "Heap after starting CLI: %d\r\n", xPortGetFreeHeapSize());
	SerialConsoleWriteString(bufferPrint);
}

/**************************************************************************/
/**
 * function          DaemonTask
 * @brief            Initialization code for all subsystems that require FreeRToS
 * @details			This function is called from the FreeRToS timer task. Any code
 *					here will be called before other tasks are initialized.
 * @param[in]        None
 * @return           None
 *****************************************************************************/

void vApplicationDaemonTaskStartupHook(void *ucParameterToPass) // vApplicationDaemonTaskStartupHook()
{

	// CODE HERE: Initialize any HW here

	// Initialize tasks
	StartTasks();
}

void vApplicationMallocFailedHook(void)
{
	SerialConsoleWriteString("Error on memory allocation on FREERTOS!\r\n");
	while (1)
		;
}

void vApplicationStackOverflowHook(void)
{
	SerialConsoleWriteString("Error on stack overflow on FREERTOS!\r\n");
	while (1)
		;
}
