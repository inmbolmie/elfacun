/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

//I2C CLOCK MANAGEMENT ROUTINES

#include "i2c_clock.h"


boolean serialLogClock = false;
boolean threadLogClock = false;

boolean firstEverClockMessage = true;
boolean leverInitialPositionIsRightHigh = true;

#define I2C_BUFFER_LENGTH 128

// I2C message descriptors
uint8_t nulldata[] = {0};
uint8_t ping[] = {80, 32, 5, 13, 70};
uint8_t centralControll[] = {16, 32, 5, 15, 72};
uint8_t mode25[] = {16, 32, 6, 11, 57, 185};
uint8_t noAutoMessage[] = {16, 32, 6, 3, 209, 135};
uint8_t endDisplay[] = {16, 32, 5, 7, 112};
uint8_t setDisplay[] = {16, 32, 21, 6, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 32, 255, 0, 3, 0, 0, 0};
uint8_t setnrun[] = {16, 32, 12, 10, 0, 10, 5, 0, 10, 5, 0, 0};


SemaphoreHandle_t xClockSemaphore;
int externalClockSetLeftHours = 0;
int externalClockSetLeftMinutes = 0;
int externalClockSetLeftSeconds = 0;
int externalClockSetRightHours = 0;
int externalClockSetRightMinutes = 0;
int externalClockSetRightSeconds = 0;
int externalClockSetCount = 0;
boolean executeSetExternalClockSetAndRun = false;
String externalClockMessage = "";
boolean executeExternalClockSetDisplay = false;
boolean executeExternalClockEndDisplay = false;



int externalClockGetWhiteHours = 0;
int externalClockGetWhiteMinutes = 0;
int externalClockGetWhiteSeconds = 0;
int externalClockGetBlackHours = 0;
int externalClockGetBlackMinutes = 0;
int externalClockGetBlackSeconds = 0;
int externalClockGetWhiteCount = 0;
int externalClockGetBlackCount = 0;
int externalClockGetLever = 0;
int externalClockGetWhiteFlag = 0;
int externalClockGetBlackFlag = 0;
boolean externalClockMessageAvailable = false;



SemaphoreHandle_t xClockSemaphoreReceive;

SemaphoreHandle_t xClockSemaphoreReceiveEnd;

// pre-calculated CRC
const char crc_table[256] = {
  0x00, 0x07, 0x0E, 0x09, 0x1C, 0x1B, 0x12, 0x15,
  0x38, 0x3F, 0x36, 0x31, 0x24, 0x23, 0x2A, 0x2D,
  0x70, 0x77, 0x7E, 0x79, 0x6C, 0x6B, 0x62, 0x65,
  0x48, 0x4F, 0x46, 0x41, 0x54, 0x53, 0x5A, 0x5D,
  0xE0, 0xE7, 0xEE, 0xE9, 0xFC, 0xFB, 0xF2, 0xF5,
  0xD8, 0xDF, 0xD6, 0xD1, 0xC4, 0xC3, 0xCA, 0xCD,
  0x90, 0x97, 0x9E, 0x99, 0x8C, 0x8B, 0x82, 0x85,
  0xA8, 0xAF, 0xA6, 0xA1, 0xB4, 0xB3, 0xBA, 0xBD,
  0xC7, 0xC0, 0xC9, 0xCE, 0xDB, 0xDC, 0xD5, 0xD2,
  0xFF, 0xF8, 0xF1, 0xF6, 0xE3, 0xE4, 0xED, 0xEA,
  0xB7, 0xB0, 0xB9, 0xBE, 0xAB, 0xAC, 0xA5, 0xA2,
  0x8F, 0x88, 0x81, 0x86, 0x93, 0x94, 0x9D, 0x9A,
  0x27, 0x20, 0x29, 0x2E, 0x3B, 0x3C, 0x35, 0x32,
  0x1F, 0x18, 0x11, 0x16, 0x03, 0x04, 0x0D, 0x0A,
  0x57, 0x50, 0x59, 0x5E, 0x4B, 0x4C, 0x45, 0x42,
  0x6F, 0x68, 0x61, 0x66, 0x73, 0x74, 0x7D, 0x7A,
  0x89, 0x8E, 0x87, 0x80, 0x95, 0x92, 0x9B, 0x9C,
  0xB1, 0xB6, 0xBF, 0xB8, 0xAD, 0xAA, 0xA3, 0xA4,
  0xF9, 0xFE, 0xF7, 0xF0, 0xE5, 0xE2, 0xEB, 0xEC,
  0xC1, 0xC6, 0xCF, 0xC8, 0xDD, 0xDA, 0xD3, 0xD4,
  0x69, 0x6E, 0x67, 0x60, 0x75, 0x72, 0x7B, 0x7C,
  0x51, 0x56, 0x5F, 0x58, 0x4D, 0x4A, 0x43, 0x44,
  0x19, 0x1E, 0x17, 0x10, 0x05, 0x02, 0x0B, 0x0C,
  0x21, 0x26, 0x2F, 0x28, 0x3D, 0x3A, 0x33, 0x34,
  0x4E, 0x49, 0x40, 0x47, 0x52, 0x55, 0x5C, 0x5B,
  0x76, 0x71, 0x78, 0x7F, 0x6A, 0x6D, 0x64, 0x63,
  0x3E, 0x39, 0x30, 0x37, 0x22, 0x25, 0x2C, 0x2B,
  0x06, 0x01, 0x08, 0x0F, 0x1A, 0x1D, 0x14, 0x13,
  0xAE, 0xA9, 0xA0, 0xA7, 0xB2, 0xB5, 0xBC, 0xBB,
  0x96, 0x91, 0x98, 0x9F, 0x8A, 0x8D, 0x84, 0x83,
  0xDE, 0xD9, 0xD0, 0xD7, 0xC2, 0xC5, 0xCC, 0xCB,
  0xE6, 0xE1, 0xE8, 0xEF, 0xFA, 0xFD, 0xF4, 0xF3
};


// calculate checksum and put it in the last byte
char crc_calc(uint8_t *buffer) {
  int i;

  char crc_result = 0;
  char length = buffer[2] - 1;

  buffer[length] = 0;

  for (i = 0; i < length; i++)
    crc_result = crc_table[ crc_result ^ buffer[i] ]; // new CRC will be the CRC of (old CRC XORed with data byte) - see http://sbs-forum.org/marcom/dc2/20_crc-8_firmware_implementations.pdf

  if (buffer[i] == crc_result)
    return 0;
  buffer[i] = crc_result;
  return -1;
}


i2c_config_t config0;
i2c_config_t config10;
i2c_config_t config10master;



//Pins 21 and 22 are dedicated for data received in slave mode at address 0
void configure_slave_addr0() {

  config0.sda_io_num = gpio_num_t(21);
  config0.sda_pullup_en = GPIO_PULLUP_ENABLE;
  config0.scl_io_num = gpio_num_t(22);
  config0.scl_pullup_en = GPIO_PULLUP_ENABLE;
  config0.mode = I2C_MODE_SLAVE;
  config0.slave.addr_10bit_en = 0;
  config0.slave.slave_addr = 0 & 0x7F;

  i2c_param_config(I2C_NUM_0, &config0);

  i2c_driver_install(
    I2C_NUM_0,
    I2C_MODE_SLAVE,
    2 * I2C_BUFFER_LENGTH,  // rx buffer length
    2 * I2C_BUFFER_LENGTH,  // tx buffer length
    0);
}

//Pins 0 and 16 are dedicated for data received in slave or master mode at address 0x10
void configure_slave_addr10() {

  config10.sda_io_num = gpio_num_t(0);
  config10.sda_pullup_en = GPIO_PULLUP_ENABLE;
  config10.scl_io_num = gpio_num_t(16);
  config10.scl_pullup_en = GPIO_PULLUP_ENABLE;
  config10.mode = I2C_MODE_SLAVE;
  config10.slave.addr_10bit_en = 0;
  config10.slave.slave_addr = 0x10 & 0x7F;
  i2c_param_config(I2C_NUM_1, &config10);

  i2c_driver_install(
    I2C_NUM_1,
    I2C_MODE_SLAVE,
    2 * I2C_BUFFER_LENGTH,  // rx buffer length
    2 * I2C_BUFFER_LENGTH,  // tx buffer length
    0);

}


void configure_master_addr10() {


  config10master.sda_io_num = gpio_num_t(0);
  config10master.sda_pullup_en = GPIO_PULLUP_ENABLE;
  config10master.scl_io_num = gpio_num_t(16);
  config10master.scl_pullup_en = GPIO_PULLUP_ENABLE;
  config10master.mode = I2C_MODE_MASTER;
  config10master.master.clk_speed = 100000;

  i2c_param_config(I2C_NUM_1, &config10master);

  i2c_driver_install(
    I2C_NUM_1,
    I2C_MODE_MASTER,
    0,
    0,
    0);

}


//Receive i2c data
int recvi2c() {
  uint8_t inputBuffer[I2C_BUFFER_LENGTH] = {0};
  uint16_t inputLen = 0;

  if (serialLogClock) Serial.println("WAITRX:");

  inputLen = i2c_slave_read_buffer(I2C_NUM_0, inputBuffer, I2C_BUFFER_LENGTH, 2000);
  if (!inputLen) {
    return 99;
  }

  if (serialLogClock) Serial.print("RECEIVED RESPONSE FROM CLOCK: ");
  for (int i = 0; i < inputLen; i++) {
    if (serialLogClock) Serial.print(inputBuffer[i], DEC);
    if (serialLogClock) Serial.print(" ");
  }
  if (serialLogClock) Serial.println("");

  delay(10);
  return 0;
}



//Send i2c data
byte writei2c(int addr, uint8_t* data, int size, boolean waitAck, boolean forceWait) {

  byte error;
  int inputLen = 0;

  if (size > 0) {
    crc_calc(data);
  }


  if (serialLogClock) Serial.print("CRC: ");
  if (serialLogClock) Serial.println(data[size], DEC);

  do {
    //Wire1.beginTransmission(addr);
    //Wire1.write(data, size);
    //error = Wire1.endTransmission();

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    //----- WRITE BYTES -----
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, addr << 1 , true);
    if (serialLogClock) Serial.print("SENDING TO CLOCK: ");
    for (int i = 1; i < size; i++) {
      i2c_master_write_byte(cmd, data[i], false);

      if (serialLogClock) Serial.print(data[i], DEC);
      if (serialLogClock) Serial.print(" ");
    }

    if (serialLogClock) Serial.println("");

    i2c_master_stop(cmd);
    //Send queued commands
    error = i2c_master_cmd_begin(I2C_NUM_1, cmd, (1000 / portTICK_RATE_MS));
    i2c_cmd_link_delete(cmd);

    if (error) {
      if (serialLogClock) Serial.print("TX TO CLOCK ERROR: ");
      if (serialLogClock) Serial.println(error, DEC);
    }


  } while ((waitAck && !forceWait ) && error != 0);



  if (waitAck && !forceWait ) {
    //Response will be received @addr 10
    if (serialLogClock) Serial.println("CONFIGURE SLAVE ADDR 10");
    i2c_driver_delete(I2C_NUM_1);
    configure_slave_addr10();

    uint8_t inputBuffer[I2C_BUFFER_LENGTH] = {0};
    uint16_t inputLen = 0;
    if (serialLogClock) Serial.println("WAIT RESPONSE FROM CLOCK ON ADDR 10");
    inputLen = i2c_slave_read_buffer(I2C_NUM_1, inputBuffer, I2C_BUFFER_LENGTH, 200);

    if (serialLogClock) Serial.print("RECEIVED ACK FROM CLOCK ON ADDR10: ");
    for (int i = 0; i < inputLen; i++) {
      if (serialLogClock) Serial.print(inputBuffer[i], HEX);
      if (serialLogClock) Serial.print(" ");
    }
    if (serialLogClock) Serial.println("");

    //Revert to MASTER
    if (serialLogClock) Serial.println("CONFIGURE MASTER ADDR 10");
    i2c_driver_delete(I2C_NUM_1);
    configure_master_addr10();

  }


  if (error == 0) {
    if (serialLogClock) Serial.println("TXOK");

    if (waitAck && forceWait) {
      //Response will be received @addr 0
      if (serialLogClock) Serial.println("WAIT RESPONSE FROM CLOCK ON ADDR 0");
      //digitalWrite(15, true);
      return recvi2c();
    }


  } else  {

    //digitalWrite(15, true);

    if (forceWait) {
      //Response will be received @addr 0
      return recvi2c();
    }

    if (serialLogClock) Serial.print("TX TO CLOCK ERROR: ");
    if (serialLogClock) Serial.println(error, HEX);
    return error;
  }
  return error;
}




//Try to initialize i2c and look for a connected clock
//Returns true if clock is found
boolean initializei2c() {

  delay(500);

  boolean initialized = false;

  //Ping the clock
  if (serialLogClock) Serial.println("PING CLOCK ON ADDR 8");
  byte error = writei2c(0x08, nulldata, 0, false, false) ;

  if (error) {
    //Not wakeup?
    if (serialLogClock) Serial.println("NO RESPONSE, WAKING UP CLOCK");
    error = writei2c(0x28, nulldata, 0, true, true);
  }

  if (!error) {
    // alive!
    //initialize
    if (serialLogClock) Serial.println("PUT CLOCK IN MODE 25");
    error = writei2c(0x08, mode25, sizeof(mode25), true, false);
    if (!error) {
      if (serialLogClock) Serial.println("CLOCK INITIALIZED");
      initialized = true;

    } else {
      if (serialLogClock) Serial.println("ERROR SETTING CLOCK MODE 25");
    }

  } else {
    if (serialLogClock) Serial.println("NO RESPONSE FROM CLOCK");
  }
  delay(100);

  // Create a mutex semaphore to manage clock communications between threads
  xClockSemaphore = xSemaphoreCreateMutex();
  xClockSemaphoreReceive = xSemaphoreCreateMutex();
  xClockSemaphoreReceiveEnd = xSemaphoreCreateMutex();

  //unlocked by default
  xSemaphoreGive( ( xClockSemaphore ) );
  xSemaphoreGive( ( xClockSemaphoreReceiveEnd ) );

  //locked by default
  xSemaphoreTake( xClockSemaphoreReceive, ( TickType_t ) portMAX_DELAY );

  return initialized;
}



void externalClockEndDisplay() {

  if (threadLogClock) Serial.println("ENTER externalClockEndDisplay");

  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    executeExternalClockEndDisplay = true;
    xSemaphoreGive( xClockSemaphore );
  }

  if (threadLogClock) Serial.println("EXIT externalClockEndDisplay");

}


void externalClockEndDisplayTask() {

  if (threadLogClock) Serial.println("ENTER externalClockEndDisplayTask");

  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {

    if (executeExternalClockEndDisplay) {

      executeExternalClockEndDisplay = false;
      xSemaphoreGive( xClockSemaphore );

      writei2c(0x08, endDisplay, sizeof(endDisplay), true, false);
    }
    else {
      xSemaphoreGive( xClockSemaphore );
    }
  }

  if (threadLogClock) Serial.println("EXIT externalClockEndDisplayTask");

}



void externalClockSetDisplay(String message) {

  if (threadLogClock) Serial.println("ENTER externalClockSetDisplay, message: ");

  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    executeExternalClockSetDisplay = true;
    externalClockMessage = message;
    xSemaphoreGive( xClockSemaphore );
  }


  if (threadLogClock) Serial.print("EXIT externalClockSetDisplay, message: ");
}


void externalClockSetDisplayTask() {

  if (threadLogClock) Serial.println("ENTER externalClockSetDisplayTask");


  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {

    if (executeExternalClockSetDisplay) {

      if (serialLogClock) Serial.println("EXECUTING SET DISPLAY TASK");

      setDisplay[16] = 0;
      setDisplay[18] = 0;

      uint8_t msg[sizeof(setDisplay)];
      memcpy(msg, setDisplay, sizeof(setDisplay) );

      for (int i = 0; i < externalClockMessage.length(); i++) {
        msg[i + 4] = externalClockMessage[i];
      }

      executeExternalClockSetDisplay = false;
      xSemaphoreGive( xClockSemaphore );

      writei2c(0x08, endDisplay, sizeof(endDisplay), true, false);
      writei2c(0x08, msg, sizeof(msg), true, false);
    }
    else {
      xSemaphoreGive( xClockSemaphore );
    }
  }


  if (threadLogClock) Serial.println("EXIT externalClockSetDisplayTask");

}




//Set and run from the app to the physical clock
void externalClockSetAndRun(int lhours, int lmins, int lsecs, boolean lCountsDown, boolean lRunning, int rhours, int rmins, int rsecs,  boolean rCountsDown, boolean rRunning) {

  if (threadLogClock) Serial.println("ENTER externalClockSetAndRun");

  if (serialLogClock) Serial.print("-------------------XXX-->SETTING EXTERNAL CLOCK TO (DEC): ");

  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    executeSetExternalClockSetAndRun = true;
    externalClockSetLeftHours = decToBcd(lhours);
    externalClockSetLeftMinutes = decToBcd(lmins);
    externalClockSetLeftSeconds = decToBcd(lsecs);
    externalClockSetRightHours = decToBcd(rhours);
    externalClockSetRightMinutes = decToBcd(rmins);
    externalClockSetRightSeconds = decToBcd(rsecs);

    if (isPhysicalClockReversed()) {
      externalClockSetLeftHours = decToBcd(rhours);
      externalClockSetLeftMinutes = decToBcd(rmins);
      externalClockSetLeftSeconds = decToBcd(rsecs);
      externalClockSetRightHours = decToBcd(lhours);
      externalClockSetRightMinutes = decToBcd(lmins);
      externalClockSetRightSeconds = decToBcd(lsecs);

    }


    if (serialLogClock) Serial.print(lhours, DEC);
    if (serialLogClock) Serial.print(" ");
    if (serialLogClock) Serial.print(lmins, DEC);
    if (serialLogClock) Serial.print(" ");
    if (serialLogClock) Serial.print(lsecs, DEC);
    if (serialLogClock) Serial.print(" ");

    if (serialLogClock) Serial.print(rhours, DEC);
    if (serialLogClock) Serial.print(" ");
    if (serialLogClock) Serial.print(rmins, DEC);
    if (serialLogClock) Serial.print(" ");
    if (serialLogClock) Serial.print(rsecs, DEC);
    if (serialLogClock) Serial.println(" ");


    int lrun = 0;
    int rrun = 0;
    
    if (lRunning && lCountsDown) lrun = 1;
    if (lRunning && !lCountsDown) lrun = 2;
    if (rRunning && rCountsDown) rrun = 4;
    if (rRunning && !rCountsDown) rrun = 8;


    if (isPhysicalClockReversed()) {

      lrun = 0;
      rrun = 0;
      
      if (rRunning && rCountsDown) lrun = 1;
      if (rRunning && !rCountsDown) lrun = 2;
      if (lRunning && lCountsDown) rrun = 4;
      if (lRunning && !lCountsDown) rrun = 8;
    }

    externalClockSetCount = lrun | rrun;

    xSemaphoreGive( xClockSemaphore );
  }

  if (threadLogClock) Serial.println("EXIT externalClockSetAndRun");

}



void externalClockSetAndRunTask() {

  if (threadLogClock) Serial.println("ENTER externalClockSetAndRunTask");

  if ( xSemaphoreTake( xClockSemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {

    if (executeSetExternalClockSetAndRun) {

      if (serialLogClock) Serial.println("EXECUTING SET AND RUN TASK");

      uint8_t msg[sizeof(setnrun)];
      memcpy(msg, setnrun, sizeof(setnrun) );

      msg[4] = externalClockSetLeftHours;
      msg[5] = externalClockSetLeftMinutes;
      msg[6] = externalClockSetLeftSeconds;
      msg[7] = externalClockSetRightHours;
      msg[8] = externalClockSetRightMinutes;
      msg[9] = externalClockSetRightSeconds;
      msg[10] = externalClockSetCount;

      executeSetExternalClockSetAndRun = false;
      xSemaphoreGive( xClockSemaphore );
      writei2c(0x08, msg, sizeof(msg), true, false);
    }
    else {
      xSemaphoreGive( xClockSemaphore );
    }
  }

  if (threadLogClock) Serial.println("EXIT externalClockSetAndRunTask");

}



//External clock management will run in RTOS core 0 in parallel to the main loop
void externalClockLoop( void * pvParameters ) {


  writei2c(0x08, setnrun, sizeof(setnrun), true, false);


  for (;;) {

    //Check for messages from the clock
    externalClockCheckMessages();

    //check tasks
    externalClockEndDisplayTask();
    externalClockSetDisplayTask();
    externalClockSetAndRunTask();

    delay(50);
  }


}


void runExternalClockLoop() {

  TaskHandle_t Task1;

  xTaskCreatePinnedToCore(
    externalClockLoop, /* Task function. */
    "TaskClock",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    0);        /* pin task to core 0 */

}





byte inputCommandPrefix[3] = {0};
int commandDataBytesToRead = 0;
int commandDataBytesRead = 0;
byte commandDataBytes[100];
int inputCommandToExecute = 0;

void externalClockCheckMessages() {


  uint8_t inputBuffer[I2C_BUFFER_LENGTH] = {0};
  uint16_t inputLen = 0;

  inputLen = i2c_slave_read_buffer(I2C_NUM_0, inputBuffer, I2C_BUFFER_LENGTH, 50);

  if (inputLen == 0) {
    // nothing received
    return;
  }

  if (inputLen > 1) {


    if (serialLogClock) Serial.print("RECEIVED TRANSMISSION FROM CLOCK (HEX): ");

    for (int i = 0; i < inputLen; i++) {

      if (serialLogClock) Serial.print(inputBuffer[i], HEX);
      if (serialLogClock) Serial.print(" ");

      if (commandDataBytesToRead == 0) {
        //Parsing for a new command
        inputCommandPrefix[0] = inputCommandPrefix[1];
        inputCommandPrefix[1] = inputCommandPrefix[2];
        inputCommandPrefix[2] = inputBuffer[i];

        //Check if inputCommandPrefix is ok for any of the known patterns
        if (inputCommandPrefix[0] == 0x10 && inputCommandPrefix[1] == 0x18 && inputCommandPrefix[2] == 0x4) {
          //Clock times message
          inputCommandToExecute = 4;
          commandDataBytesToRead = 20;
          commandDataBytesRead = 0;
        }

        else if (inputCommandPrefix[0] == 0x10 && inputCommandPrefix[1] == 0xA && inputCommandPrefix[2] == 0x5) {
          //Button change message
          inputCommandToExecute = 5;
          commandDataBytesToRead = 8;
          commandDataBytesRead = 0;
        }

        else if (inputCommandPrefix[0] == 0x10 && inputCommandPrefix[1] == 0x8 && inputCommandPrefix[2] == 0x1) {
          //Button change message
          inputCommandToExecute = 1;
          commandDataBytesToRead = 4;
          commandDataBytesRead = 0;
        }

      }

      else {
        //Command being read, add to buffer
        commandDataBytes[commandDataBytesRead++] = inputBuffer[i];

        if (commandDataBytesRead == commandDataBytesToRead) {
          //Command complete
          commandDataBytesRead = 0;
          commandDataBytesToRead = 0;
          switch (inputCommandToExecute) {
            case 1:
              if (serialLogClock) Serial.println("----------->COMMAND 1");
              processAckReceivedTask(commandDataBytes);
              break;
            case 5:
              if (serialLogClock) Serial.println("----------->COMMAND 5");
              processButtonChangeReceivedTask(commandDataBytes);
              break;
            case 4:
              if (serialLogClock) Serial.println("----------->COMMAND 4");
              processTimeReceivedTask(commandDataBytes);
              break;
          }
          inputCommandToExecute = 0;
          inputCommandPrefix[0] = 0;
          inputCommandPrefix[1] = 0;
          inputCommandPrefix[0] = 0;
        }
      }
    }
    if (serialLogClock) Serial.println("");
  }
}




void checkForPendingExternalClockMessages() {

  if (threadLogClock) Serial.println("SEMAPHORE checkForPendingExternalClockMessages");
  //unlock receive
  xSemaphoreGive( ( xClockSemaphoreReceive ) );

  while ( xSemaphoreTake( xClockSemaphoreReceiveEnd, ( TickType_t ) portMAX_DELAY ) != pdTRUE ) {}

  //lock again
  xSemaphoreTake( xClockSemaphoreReceive, ( TickType_t ) portMAX_DELAY );

  //unlock other methods
  xSemaphoreGive( ( xClockSemaphoreReceiveEnd ) );
  if (threadLogClock) Serial.println("SEMAPHORE EXIT checkForPendingExternalClockMessages");
}



void processAckReceivedTask(byte* commandDataBytes) {

  //Do nothing
  //
  //  if ( xSemaphoreTake( xClockSemaphoreReceive, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  //  {
  //    processPendingExternalClockMessageAck();
  //    xSemaphoreGive( ( xClockSemaphoreReceive ) );
  //  }

}


void sendButtonPushCommand(int button) {

  if (threadLogClock) Serial.println("SEMAPHORE sendButtonPushCommand");

  while ( xSemaphoreTake( xClockSemaphoreReceiveEnd, ( TickType_t ) portMAX_DELAY ) != pdTRUE ) {}
  while ( xSemaphoreTake( xClockSemaphoreReceive, ( TickType_t ) portMAX_DELAY ) != pdTRUE ) {}

  processPendingExternalClockMessageButtonChange(button);

  xSemaphoreGive( ( xClockSemaphoreReceive ) );
  xSemaphoreGive( ( xClockSemaphoreReceiveEnd ) );



  if (threadLogClock) Serial.println("SEMAPHORE EXIT sendButtonPushCommand");
}



void processButtonChangeReceivedTask(byte* commandDataBytes) {

  if (threadLogClock) Serial.print("commandDataBytes: ");
  if (threadLogClock) Serial.print(commandDataBytes[0], HEX);
  if (threadLogClock) Serial.print(" ");
  if (threadLogClock) Serial.println(commandDataBytes[1], HEX);

  //Decode the stuff
  if (!(commandDataBytes[1] & 0x01) && (commandDataBytes[0] & 0x01) ) {
    sendButtonPushCommand(BUTTON_BACK) ;
  }
  if (!(commandDataBytes[1] & 0x02 ) && (commandDataBytes[0] & 0x02 )) {
    sendButtonPushCommand(BUTTON_PLUS) ;
  }
  if (!(commandDataBytes[1] & 0x04 ) && (commandDataBytes[0] & 0x04 )) {
    sendButtonPushCommand(BUTTON_PLAY) ;
  }
  if (!(commandDataBytes[1] & 0x08 ) && (commandDataBytes[0] & 0x08 )) {
    sendButtonPushCommand(BUTTON_MINUS) ;
  }
  if (!(commandDataBytes[1] & 0x10 ) && (commandDataBytes[0] & 0x10 )) {
    sendButtonPushCommand(BUTTON_NEXT) ;
  }

  if (!(commandDataBytes[1] & 0x40 ) && (commandDataBytes[0] & 0x40 )) {
    sendButtonPushCommand(BUTTON_LEVER) ;
  }

  if (!(commandDataBytes[0] & 0x40 ) && (commandDataBytes[1] & 0x40 )) {
    sendButtonPushCommand(BUTTON_LEVER) ;
  }
}



boolean isPhysicalClockReversed() {
  return leverInitialPositionIsRightHigh ^ boardInverted;
}


//Time received from physical clock
void processTimeReceivedTask(byte* commandDataBytes) {

  ////Decode the stuff

  int lhours = commandDataBytes[1] & 0x0f;
  int lmins = commandDataBytes[2];
  int lsecs = commandDataBytes[3];
  int rhours = commandDataBytes[7] & 0x0f;
  int rmins = commandDataBytes[8];
  int rsecs = commandDataBytes[9];
  int leverState = commandDataBytes[15] & 1;
  int updt = commandDataBytes[20];


 //Try to determine the clock orientation based on the lever position, when started the lever should be up at white's side
  if (firstEverClockMessage) {
    leverInitialPositionIsRightHigh = leverState;
    firstEverClockMessage = false;
    if (serialLog) Serial.print("LEVER INITIAL POSITION HIGH RIGHT: ");
    if (serialLog) Serial.println(leverInitialPositionIsRightHigh, DEC);
  }

  if (isPhysicalClockReversed()) {

    int rhours = commandDataBytes[1] & 0x0f;
    int rmins = commandDataBytes[2];
    int rsecs = commandDataBytes[3];
    int lhours = commandDataBytes[7] & 0x0f;
    int lmins = commandDataBytes[8];
    int lsecs = commandDataBytes[9];
  }

  if (threadLogClock) Serial.println("SEMAPHORE processTimeReceivedTask");

  while ( xSemaphoreTake( xClockSemaphoreReceiveEnd, ( TickType_t ) portMAX_DELAY ) != pdTRUE ) {}
  while ( xSemaphoreTake( xClockSemaphoreReceive, ( TickType_t ) portMAX_DELAY ) != pdTRUE ) {}

  processPendingExternalClockMessageTime(lhours, lmins, lsecs, rhours, rmins, rsecs, leverState, updt);

  xSemaphoreGive( ( xClockSemaphoreReceive ) );
  xSemaphoreGive( ( xClockSemaphoreReceiveEnd ) );


  if (threadLogClock) Serial.println("SEMAPHORE EXIT processTimeReceivedTask");



}
