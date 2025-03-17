# a07g-exploring-the-CLI

* Team Number: T06
* Team Name: Byte Crafter
* Team Members: Tony Yan & Yue Zhang
* GitHub Repository URL: https://github.com/ese5160/final-project-a07g-a14g-t06-byte-crafter
* Description of test hardware: ROG Zephyrus G14, HUAWEI 14

## 1. Software Architecture

## 2. Understanding the Starter Code
### 2.1
*InitializeSerialConsole()* function is used to initialize the UART and register the callback function, the main purpose is to prepare the serial port for communication: 1. Initialize Ring Buffers (Circular Buffers); 2. Configure USART (Universal Synchronous/Asynchronous Transceiver); 3. Setting Interrupt Priority; 4. Initiate Serial Data Read.

cbufRx is a Circular Buffer that points to the rxCharacterBuffer, which is used to store data received from the serial port;
cbufTx is a Circular Buffer that points to the txCharacterBuffer, which holds the data to be sent.

Data structure: FIFO

### 2.2

cbufRx and cbufTx are initialized by *circular_buf_init()*, which allocates a *circular_buf_t* structure and binds the address pointed to by the pointer to rxCharacterBuffer and txCharacterBuffer as a data store by means of pointers to rxCharacterBuffer and txCharacterBuffer respectively, which are used to store the serial port data and set the maximum storage size of the circular buffer according to the array size. It also need to clear the buffer.

### 2.3

### 2.4

### 2.5


### 2.6

### 2.7


### 2.8


### 2.9

## 3. Debug Logger Module

## 4. Wiretap the convo

## 5. Complete the CLI

## 6. Add CLI commands

## 7. Using Percepio
