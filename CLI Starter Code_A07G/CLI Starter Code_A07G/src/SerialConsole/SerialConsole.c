/**************************************************************************/
/**
 * @file        SerialConsole.c
 * @ingroup 	Serial Console
 * @brief       This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 * @details     This file has the code necessary to run the CLI and Serial Debugger. 
 * 				It initializes an UART channel and uses it to receive command from the user
 *				as well as print debug information.
 *
 *				The code in this file will:
 *				--Initialize a SERCOM port (SERCOM # ) to be an UART channel operating at 115200 baud/second, 8N1
 *				--Register callbacks for the device to read and write characters asynchronously as required by the CLI
 *				--Initialize the CLI and Debug Logger data structures
 *
 *				Usage:
 *
 *
 * @copyright
 * @author
 * @date        January 26, 2019
 * @version		0.1
 *****************************************************************************/

/******************************************************************************
 * Includes
 ******************************************************************************/
#include "SerialConsole.h"

/******************************************************************************
 * Defines
 ******************************************************************************/
#define RX_BUFFER_SIZE 512 ///< Size of character buffer for RX, in bytes
#define TX_BUFFER_SIZE 512 ///< Size of character buffers for TX, in bytes

/******************************************************************************
 * Structures and Enumerations
 ******************************************************************************/
cbuf_handle_t cbufRx; ///< Circular buffer handler for receiving characters
cbuf_handle_t cbufTx; ///< Circular buffer handler for transmitting characters

char latestRx; ///< Holds the latest character received
char latestTx; ///< Holds the latest character to be transmitted

/******************************************************************************
 * Callback Declarations
 ******************************************************************************/
void usart_write_callback(struct usart_module *const usart_module); // Callback for when we finish writing characters to UART
void usart_read_callback(struct usart_module *const usart_module); // Callback for when we finis reading characters from UART

/******************************************************************************
 * Local Function Declarations
 ******************************************************************************/
static void configure_usart(void);
static void configure_usart_callbacks(void);

/******************************************************************************
 * Global Variables
 ******************************************************************************/
struct usart_module usart_instance;
char rxCharacterBuffer[RX_BUFFER_SIZE]; 			   ///< Buffer to store received characters
char txCharacterBuffer[TX_BUFFER_SIZE]; 			   ///< Buffer to store characters to be sent
enum eDebugLogLevels currentDebugLevel = LOG_INFO_LVL; ///< Default debug level

static SemaphoreHandle_t xRxSemaphore = NULL;  // Semaphore
static SemaphoreHandle_t xRxMutex = NULL;      // Mutex

/******************************************************************************
 * Global Functions
 ******************************************************************************/

/**
 * @brief Initializes the UART and registers callbacks.
 */
void InitializeSerialConsole(void)
{
    // Initialize circular buffers for RX and TX
	cbufRx = circular_buf_init((uint8_t *)rxCharacterBuffer, RX_BUFFER_SIZE);
    cbufTx = circular_buf_init((uint8_t *)txCharacterBuffer, TX_BUFFER_SIZE);

    // Configure USART and Callbacks
	configure_usart();
    configure_usart_callbacks();
    NVIC_SetPriority(SERCOM4_IRQn, 10);

    usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1); // Kicks off constant reading of characters

	// Add any other calls you need to do to initialize your Serial Console
	// Create semaphore
	xRxSemaphore = xSemaphoreCreateBinary();
	if (xRxSemaphore == NULL)
	{
		SerialConsoleWriteString("ERROR: Could not create RX semaphore!");
	}
}

/**
 * @brief Deinitializes the UART.
 */
void DeinitializeSerialConsole(void)
{
    usart_disable(&usart_instance);
}

/**
 * @brief Writes a string to the UART.
 * @param string Pointer to the string to send.
 */
void SerialConsoleWriteString(char *string)
{
    if (string != NULL)
    {
        for (size_t iter = 0; iter < strlen(string); iter++)
        {
            circular_buf_put(cbufTx, string[iter]);
        }

        if (usart_get_job_status(&usart_instance, USART_TRANSCEIVER_TX) == STATUS_OK)
        {
            circular_buf_get(cbufTx, (uint8_t *)&latestTx); // Perform only if the SERCOM TX is free (not busy)
            usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
        }
    }
}

/**
 * @brief Reads a character from the RX buffer.
 * @param rxChar Pointer to store the received character.
 * @return -1 if buffer is empty, otherwise the character read.
 */
int SerialConsoleReadCharacter(uint8_t *rxChar)
{
    vTaskSuspendAll();
    int a = circular_buf_get(cbufRx, (uint8_t *)rxChar);
    xTaskResumeAll();
    return a;
}

/**
 * @brief Gets the current debug log level.
 * @return The current debug level.
 */
enum eDebugLogLevels getLogLevel(void)
{
    return currentDebugLevel;
}

/**
 * @brief Sets the debug log level.
 * @param debugLevel The debug level to set.
 */
void setLogLevel(enum eDebugLogLevels debugLevel)
{
    currentDebugLevel = debugLevel;
}

/**
 * @brief Logs a message at the specified debug level.
 * 
 * Prints a formatted log message to the console, prefixed with the log level.
 * The message will only be printed if the specified log level is greater than or 
 * equal to the current log level.
 * 
 * @param level The log level for the message.
 * @param format The format string for the message (like printf).
 * @param ... Additional arguments for formatting.
 */
void LogMessage(enum eDebugLogLevels level, const char *format, ...)
{
    // Todo: Implement Debug Logger
	// More detailed descriptions are in header file
	
	// Skip message if the log level is lower than the current level
	if (level < currentDebugLevel) {
		return;
	}
	
	switch (level) {
		case LOG_INFO_LVL:    SerialConsoleWriteString("[INFO] ");; break;
		case LOG_DEBUG_LVL:   SerialConsoleWriteString("[DEBUG] ");; break;
		case LOG_WARNING_LVL: SerialConsoleWriteString("[WARNING] "); break;
		case LOG_ERROR_LVL:   SerialConsoleWriteString("[ERROR] "); break;
		case LOG_FATAL_LVL:   SerialConsoleWriteString("[FATAL] "); break;
		case LOG_OFF_LVL:     return;  
		default:              SerialConsoleWriteString("[UNKNOWN] "); break;
	}
	
	// Initialize variable argument list
	va_list args;
	va_start(args, format);

	// Print the formatted message
	char buffer[256];
	vsnprintf(buffer, sizeof(buffer), format, args);

	// Clean up the variable argument list
	va_end(args);

	SerialConsoleWriteString(buffer);
	SerialConsoleWriteString("\r\n");	
}

/*
COMMAND LINE INTERFACE COMMANDS
*/

/******************************************************************************
 * Local Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			static void configure_usart(void)
 * @brief		Code to configure the SERCOM "EDBG_CDC_MODULE" to be a UART channel running at 115200 8N1
 * @note
 *****************************************************************************/
static void configure_usart(void)
{
	struct usart_config config_usart;
	usart_get_config_defaults(&config_usart);

	config_usart.baudrate = 115200;
	config_usart.mux_setting = EDBG_CDC_SERCOM_MUX_SETTING;
	config_usart.pinmux_pad0 = EDBG_CDC_SERCOM_PINMUX_PAD0;
	config_usart.pinmux_pad1 = EDBG_CDC_SERCOM_PINMUX_PAD1;
	config_usart.pinmux_pad2 = EDBG_CDC_SERCOM_PINMUX_PAD2;
	config_usart.pinmux_pad3 = EDBG_CDC_SERCOM_PINMUX_PAD3;
	while (usart_init(&usart_instance,
					  EDBG_CDC_MODULE,
					  &config_usart) != STATUS_OK)
	{
	}

	usart_enable(&usart_instance);
}

/**************************************************************************/ 
/**
 * @fn			static void configure_usart_callbacks(void)
 * @brief		Code to register callbacks
 * @note
 *****************************************************************************/
static void configure_usart_callbacks(void)
{
	usart_register_callback(&usart_instance,
							usart_write_callback,
							USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_register_callback(&usart_instance,
							usart_read_callback,
							USART_CALLBACK_BUFFER_RECEIVED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_TRANSMITTED);
	usart_enable_callback(&usart_instance, USART_CALLBACK_BUFFER_RECEIVED);
}

/******************************************************************************
 * Callback Functions
 ******************************************************************************/

/**************************************************************************/ 
/**
 * @fn			void usart_read_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes receives all the bytes requested from a UART read job
		 Students to fill out. Please note that the code here is dummy code. It is only used to show you how some functions work.
 * @note
 *****************************************************************************/
void usart_read_callback(struct usart_module *const usart_module)
{
	// ToDo: Complete this function
	
	//Flag used to indicate whether a higher priority task was woken by the ISR
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	
	if (circular_buf_put2(cbufRx, latestRx) == 0)
	{
		xSemaphoreGiveFromISR(xRxSemaphore, &xHigherPriorityTaskWoken);
	}
	
	// Make sure to enable the next receiving
	while (usart_read_buffer_job(&usart_instance, (uint8_t *)&latestRx, 1) != STATUS_OK);
	
	// If a higher priority task was woken, yield to it
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}


/**************************************************************************/ 
/**
 * @fn			void usart_write_callback(struct usart_module *const usart_module)
 * @brief		Callback called when the system finishes sending all the bytes requested from a UART read job
 * @note
 *****************************************************************************/
void usart_write_callback(struct usart_module *const usart_module)
{
	if (circular_buf_get(cbufTx, (uint8_t *)&latestTx) != -1) // Only continue if there are more characters to send
	{
		usart_write_buffer_job(&usart_instance, (uint8_t *)&latestTx, 1);
	}
}

/**
 * @brief Retrieves the RX semaphore used to signal character reception from UART.
 * @return SemaphoreHandle_t - A handle to the RX semaphore. May be NULL if not initialized.
 */
SemaphoreHandle_t GetSerialRxSemaphore(void)
{
	return xRxSemaphore;
}
