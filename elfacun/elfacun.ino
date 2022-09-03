/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#define AA_FONT_SMALL "NotoSansBold15"
#define FS_NO_GLOBALS

#include "board.h"
#include "board_a.h"
#include "board_b.h"
#include "rfid.h"
#include "soc/rtc.h"
#include "ble_board_b.h"
#include "i2c_clock.h"
#include "screen.h"
#include "serial.h"
#include "nvs_conf.h"
#include "system_update.h"
#include "lichess.h"
#include "driver/adc.h"

#define SPI_FREQUENCY  1000000
#define SPI_READ_FREQUENCY 500000


#include <FS.h>
#include "SPIFFS.h"
#include <SPI.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "esp_system.h"


#define INCLUDE_vTaskSuspend


// These are used to get information about static SRAM and flash memory sizes
extern "C" char __data_start[];    // start of SRAM data
extern "C" char _end[];     // end of SRAM data (used to check amount of SRAM this program's variables use)
extern "C" char __data_load_end[];  // end of FLASH (used to check amount of Flash this program's code and data uses)

int numloop = 0;

boolean firstScan = true;

//Hardware initialization
void setup() {

  //ENABLE BOARD BUS
  pinMode(EN_BUS, OUTPUT);
  digitalWrite(EN_BUS, false);

  //clear led state boards
  clearAllBoardLeds();

  //disable ESP32 brownout detector, works really bad with non-usb power
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  //Configure comms to board in default values
  pinMode(CONTROLLER_OUTPUT_DISABLE, OUTPUT);
  //disable PIC to bus
  digitalWrite(CONTROLLER_OUTPUT_DISABLE, true);

  //disable passive mode latches
  pinMode(LATCH_ROWS_DISABLE, OUTPUT);
  pinMode(LATCH_COLUMNS_DISABLE, OUTPUT);
  digitalWrite(LATCH_ROWS_DISABLE, true);
  digitalWrite(LATCH_COLUMNS_DISABLE, true);

  pinMode(CONTROLLER_OUTPUT_CHIPSELECT, OUTPUT);
  digitalWrite(CONTROLLER_OUTPUT_CHIPSELECT, true);

  pinMode(CONTROLLER_OUTPUT_CHIPSELECT_BUTTONS, OUTPUT);
  digitalWrite(CONTROLLER_OUTPUT_CHIPSELECT_BUTTONS, true);

  pinMode(RFID_CS, OUTPUT);
  digitalWrite(RFID_CS, true);

  //initialize SPI to communicate with the screen, RFID, SD card and SPI expanders.
  SPI.begin();

  //initialize screen
  initialize_tft();

  //SPI.setFrequency(10000000);

  //initialize SPI port expanders
  SPIExpander.begin();
  delay(100);
  SPIExpanderButtons.begin();

  //Set expander ports
  //INPUT PORT
  SPIExpander.pinMode(0, INPUT);
  SPIExpander.pinMode(1, INPUT);
  SPIExpander.pinMode(2, INPUT);
  SPIExpander.pinMode(3, INPUT);
  SPIExpander.pinMode(4, INPUT);
  SPIExpander.pinMode(5, INPUT);
  SPIExpander.pinMode(6, INPUT);
  SPIExpander.pinMode(7, INPUT);

  //OUTPUT PORT
  SPIExpander.pinMode(8, OUTPUT);
  SPIExpander.pinMode(9, OUTPUT);
  SPIExpander.pinMode(10, OUTPUT);
  SPIExpander.pinMode(11, OUTPUT);
  SPIExpander.pinMode(12, OUTPUT);
  SPIExpander.pinMode(13, OUTPUT);
  SPIExpander.pinMode(14, OUTPUT);
  SPIExpander.pinMode(15, OUTPUT);

  //BUTTONS
  SPIExpanderButtons.pinMode(0, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(1, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(2, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(3, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(4, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(5, INPUT_PULLUP);
  SPIExpanderButtons.pinMode(6, INPUT);
  SPIExpanderButtons.pinMode(7, INPUT);

  //BUS CONTROL SIGNALS
  SPIExpanderButtons.pinMode(8, OUTPUT);
  SPIExpanderButtons.pinMode(9, OUTPUT);
  SPIExpanderButtons.pinMode(10, OUTPUT);
  SPIExpanderButtons.pinMode(11, OUTPUT);
  SPIExpanderButtons.pinMode(12, INPUT);
  SPIExpanderButtons.pinMode(13, INPUT);
  SPIExpanderButtons.pinMode(14, INPUT);
  SPIExpanderButtons.pinMode(15, INPUT);

  //Put the bus in steaady mode
  busValuesToDefault();

  //initialize nvs keystore to store/read overridden configuration parameters
  initialize_nvs();

  //SD card chipselect
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, true);

  //adc_power_off();
  //WiFi.disconnect(true);  // Disconnect from the network
  //WiFi.mode(WIFI_OFF);    // Switch WiFi off
  //setCpuFrequencyMhz(160);

  //tft.println("CHECK SD");

  //Check SD card presence
  //Does not work with ESP 2.0.0-2.0.2 (?)
  if (SD.begin(SD_CS)) {

    uint8_t cardType;
    cardType = SD.cardType();

    if (cardType == CARD_NONE) {
      tft.println("SD card not found");
      delay(500);
    } else {
      //Update system firmware from SD if update.bin is present
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(0, 0);
      updateFromFS(SD);
    }


    //Overwrite nvs config from sd if settings.txt is present in the SD
    read_config_sd_store_nvs();

    //Overwrite SPIFFS files if SPIFFS folder is present in the SD
    //TBD

    SD.end();

  } else {
    tft.println("SD CARD NOT FOUND");
    delay(100);
  }

  //Disable SD card
  digitalWrite(SD_CS, true);


  //setCpuFrequencyMhz(240);
  //adc_power_on();
  //WiFi.disconnect(false);  // Reconnect the network
  //WiFi.mode(WIFI_STA);    // Switch WiFi off


  //Clear screen
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  //tft.setTextColor(TFT_WHITE); tft.setTextSize(1);

  //Read config from NVS if has been stored there at any time from SD card
  read_config_nvs();

  //initialize RFID reader/writer
  rfid_init();


  //Load custom base MAC address for BT if defined
  //BT address will be base MAC plus 2
  tft.print("BASE MAC: ");
  tft.println(baseMacAddress);
  if (baseMacAddress != "") {
    uint8_t new_mac[8];
    sscanf(baseMacAddress, "%2x:%2x:%2x:%2x:%2x:%2x", &new_mac[0], &new_mac[1], &new_mac[2], &new_mac[3], &new_mac[4], &new_mac[5]);
    esp_base_mac_addr_set(new_mac);
    tft.printf("Mac assigned: %2x:%2x:%2x:%2x:%2x:%2x\n", new_mac[0], new_mac[1], new_mac[2], new_mac[3], new_mac[4], new_mac[5]);
  } else {
    tft.println("Setting default mac");
  }

  //Initialize SPIFFS to get bmp files for displaying
  SPIFFS.begin();

  //Decide the mode we will initialize in.
  //A-> Serial port @ 9600 bauds + BT
  //B-> Serial port @ 38400 bauds + BT
  //C-> Lichess over WIFI
  //Modes A and B are incompatible over USB as mode a is 9600 bauds and mode B is 38400 bauds.
  //Modes A and B should be compatible over Bluetooth, you can connect either A or B board regardless of the mode we boot on
  //BLE can be used in both A and B modes
  //
  //So the difference between booting in A or B mode is:
  //A mode-> serial speed is 9600, Bluetooth advertised name is mode.a.bt.advertised.name
  //B mode-> serial speed is 38400, Bluetooth advertised name is mode.b.bt.advertised.name

  boolean noButtonsPushed = SPIExpanderButtons.digitalRead(0) && SPIExpanderButtons.digitalRead(1) && SPIExpanderButtons.digitalRead(2);


  if ((noButtonsPushed && strcmp(defaultMode, "C") == 0) || !SPIExpanderButtons.digitalRead(2)) {
    //For Lichess
    Serial.begin(9600);

    displayLichessLogo();

    if (serialLog) Serial.println("Init USB lichess mode");

    initializeWifi();
    delay(2000);
    tft.setCursor(0, 0);

    if (serialLog) Serial.println("Searching for running games");
    while (!getRunningGameLichess()) {
      if (serialLog) Serial.println("Retrying getRunningGameLichess");
    }
    isLichessConnected = true;

  }

  else if ((noButtonsPushed && strcmp(defaultMode, "B") == 0) || !SPIExpanderButtons.digitalRead(1)) {
    //For type B board
    Serial.begin(38400, SERIAL_7O1);
    displayModeBLogo();
    SerialBT.begin(modeBBtAdvertisedName);
    isTypeBUSBBoard = true;
    initBleService();
    if (serialLog) Serial.println("Init USB mode B");

  } else {
    //For type A board
    Serial.begin(9600);
    displayModeALogo();
    SerialBT.begin(modeABtAdvertisedName);
    initBleService();
    if (serialLog) Serial.println("Init USB mode A");

  }

  //Sound will be disabled if we boot pushing the change lever button
  if ((!SPIExpanderButtons.digitalRead(5) && buzzerEnabled == "TRUE") || (SPIExpanderButtons.digitalRead(5) && buzzerEnabled == "FALSE")) {
    if (serialLog) Serial.println("Disabling sound from button");
    soundIsDisabled = true;
  }


  if ((!SPIExpanderButtons.digitalRead(5) && buzzerEnabled == "FALSE")) {
    if (serialLog) Serial.println("Enabling sound from button");
    soundIsDisabled = false;
  }


  //Leds will be disabled if we boot pushing the next button
  if ((!SPIExpanderButtons.digitalRead(4) && ledsEnabled == "TRUE")) {
    if (serialLog) Serial.println("Disabling leds from button");
    ledsAreDisabled = true;
  }


  if ((!SPIExpanderButtons.digitalRead(4) && ledsEnabled == "FALSE")) {
    if (serialLog) Serial.println("Enabling leds from button");
    ledsAreDisabled = false;
  }

  //LOG CPU SPEED
  if (serialLog) Serial.print("ESP32 CPU SPEED: ");
  if (serialLog) Serial.println(rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get()), DEC);

  //Clear screen
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);



  //Setup board initial position with zeroes
  memset(&currentPos, 0, sizeof(initpos[0]) * 64);
  promotingSquare = 100;
  boardInInitialPos = false;
  boardInverted = false;

  tft.loadFont(AA_FONT_SMALL);


  //Timer initialization for clock handling
  //Timers are used for local chess clock and led handling

  // Use 1st timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timer = timerBegin(0, 80, true);
  timerLed = timerBegin(1, 80, true);

  // Attach onTimer function to our timers
  //clock timer
  timerAttachInterrupt(timer, &onTimer, true);
  //led timer
  timerAttachInterrupt(timerLed, &onTimerLed, true);

  // Set alarm to call onTimer function every 50ms (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timer, 50000, true);
  timerAlarmWrite(timerLed, 50000, true);

  // Start an alarm
  timerAlarmEnable(timer);
  timerAlarmEnable(timerLed);

  //Initialize I2C buses used to communicate with external chess clock via the jack connector
  configure_slave_addr0();
  configure_master_addr10();

  //Initialize buzzer
  pinMode(BUZZER, OUTPUT);
  digitalWrite(BUZZER, true);
  ledcSetup(0, 1E5, 12);
  ledcAttachPin(BUZZER, 0);

  boop();
  beep();

  //Look for an additional chess module plugged into the board and enter passive mode if we find one
  //To find external modules we look for activity in the bus control lines for some seconds
  //In passive mode we won't be actively managing neither board scanning nor led lighting
  //that will be controlled by the other module and we will passively listen to the signals

  isPassiveMode = checkForExternalModule();


  if (isPassiveMode) {
    //Passive mode. An original module will control the leds and board scanning and we will listen passively and capture the data
    if (serialLog) Serial.println("Entering PASSIVE mode");

    //23S17 ports are input
    SPIExpanderButtons.pinMode(LED_WR_DISABLE_COLUMN_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(LED_WR_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(ROW_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(COLUMN_DISABLE_EX, INPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_A, INPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_B, INPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_C, INPUT);

    //Data pins are always INPUT
    digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus

    //REVERSE OUTPUT PORT TO READ PASSIVE MODE LATCHES
    SPIExpander.pinMode(8, INPUT);
    SPIExpander.pinMode(9, INPUT);
    SPIExpander.pinMode(10, INPUT);
    SPIExpander.pinMode(11, INPUT);
    SPIExpander.pinMode(12, INPUT);
    SPIExpander.pinMode(13, INPUT);
    SPIExpander.pinMode(14, INPUT);
    SPIExpander.pinMode(15, INPUT);


  } else {

    //Active mode, no other module is present so we control the leds and board scanning actively
    //23S17 ports back to output
    if (serialLog) Serial.println("Entering ACTIVE mode");

    SPIExpanderButtons.pinMode(LED_WR_DISABLE_COLUMN_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(LED_WR_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(ROW_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(COLUMN_DISABLE_EX, OUTPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_A, OUTPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_B, INPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_C, OUTPUT);

    //FREEZE EXTERNAL DISPLAY
    SPIExpanderButtons.digitalWrite(DISPLAY_CTRL_A, false);
    SPIExpanderButtons.digitalWrite(DISPLAY_CTRL_C, false);

    busValuesToDefault();
  }


  //Check for external clock presence over i2c bus
  //If we find one the time data and text messages will be displayed there
  isExternalClock = initializei2c();

  if (isExternalClock) {

    runExternalClockLoop();
    if (serialLog) Serial.println("External clock initialized");
    drawBmp("/clock.bmp", 285, 10);
  } else {
    if (serialLog) Serial.println("External clock not found");
  }
}




//Main program loop
void loop() {

  //Check if Bluetooth is connected
  if (!isLichessConnected) {
    btConnected = SerialBT.connected(10);
  }

  if ((!isLichessConnected && SerialBT.available()) || Serial.available())
  {
    //noInterrupts();
    byte rcv;

    rcv = readSerial();


    //Process incoming opcodes

    //Check if we are connected to a type B board via USB. If so acivate that mode. In any other case we assume the board is type A
    if (firstCharacterReceived && (((rcv & 0x7F) == 0x56) || ((rcv & 0x7F) == 0x57))) {
      isTypeBUSBBoard = true;
      if (serialLog) Serial.println("Mode B activated");
    }

    firstCharacterReceived = false;

    if (!isTypeBUSBBoard) {
      //Type A board, we call type a command loop
      board_a_command_loop(rcv) ;

    } else {
      //Type B board, we call type b command loop
      board_b_command_loop(rcv) ;

    }
  }



  //Read board state passively reading the bus if on PASSIVE mode. We will loop waiting for data for the eight rows that usually we will get in sequence
  int numRowsScanned = 0;
  if (isPassiveMode) {

    byte rowsScanned = 0xFF;
    byte scannedRowValue = 0;
    byte scannedColValue = 0;

    //if (serialLog) Serial.println("------------->SCANNING ROWS");

    while (rowsScanned != 0x00) {
      numRowsScanned++;

      //Block until external latch inhibition signal fires to indicate latches are freshly loaded
      while (SPIExpanderButtons.digitalRead(INHIB_INV)) {
        //delay(1);
      }
      delayMicroseconds(20);

      digitalWrite(LATCH_ROWS_DISABLE, false);
      //delay(1);
      scannedRowValue = SPIExpander.readPort(1);
      digitalWrite(LATCH_ROWS_DISABLE, true);
      digitalWrite(LATCH_COLUMNS_DISABLE, false);
      //delay(1);
      scannedColValue = SPIExpander.readPort(1);
      digitalWrite(LATCH_COLUMNS_DISABLE, true);


      byte encodedRowValue = 8;
      if (scannedRowValue == 0b11111110) encodedRowValue = 7;
      if (scannedRowValue == 0b11111101) encodedRowValue = 6;
      if (scannedRowValue == 0b11111011) encodedRowValue = 5;
      if (scannedRowValue == 0b11110111) encodedRowValue = 4;
      if (scannedRowValue == 0b11101111) encodedRowValue = 3;
      if (scannedRowValue == 0b11011111) encodedRowValue = 2;
      if (scannedRowValue == 0b10111111) encodedRowValue = 1;
      if (scannedRowValue == 0b01111111) encodedRowValue = 0;


      if (encodedRowValue > 7) {
        //Error
        //        if (serialLog) Serial.print("ERROR :Invalid row value scanned: ");
        //        if (serialLog) Serial.println(scannedRowValue, BIN);

      }

      else {

        if (!boardInverted) {
          //reverse scan positions
          scannedpos[(8 * encodedRowValue) + 0 ] = !bitRead(scannedColValue, 0);
          scannedpos[(8 * encodedRowValue) + 1 ] = !bitRead(scannedColValue, 1);
          scannedpos[(8 * encodedRowValue) + 2 ] = !bitRead(scannedColValue, 2);
          scannedpos[(8 * encodedRowValue) + 3 ] = !bitRead(scannedColValue, 3);
          scannedpos[(8 * encodedRowValue) + 4 ] = !bitRead(scannedColValue, 4);
          scannedpos[(8 * encodedRowValue) + 5 ] = !bitRead(scannedColValue, 5);
          scannedpos[(8 * encodedRowValue) + 6 ] = !bitRead(scannedColValue, 6);
          scannedpos[(8 * encodedRowValue) + 7 ] = !bitRead(scannedColValue, 7);

        } else {
          scannedpos[63 - (8 * encodedRowValue) + 0 ] = !bitRead(scannedColValue, 0);
          scannedpos[63 - (8 * encodedRowValue) + 1 ] = !bitRead(scannedColValue, 1);
          scannedpos[63 - (8 * encodedRowValue) + 2 ] = !bitRead(scannedColValue, 2);
          scannedpos[63 - (8 * encodedRowValue) + 3 ] = !bitRead(scannedColValue, 3);
          scannedpos[63 - (8 * encodedRowValue) + 4 ] = !bitRead(scannedColValue, 4);
          scannedpos[63 - (8 * encodedRowValue) + 5 ] = !bitRead(scannedColValue, 5);
          scannedpos[63 - (8 * encodedRowValue) + 6 ] = !bitRead(scannedColValue, 6);
          scannedpos[63 - (8 * encodedRowValue) + 7 ] = !bitRead(scannedColValue, 7);
        }

        rowsScanned = rowsScanned & scannedRowValue;

        //        if (serialLog) Serial.print("ROW VALUE: ");
        //        if (serialLog) Serial.print(scannedRowValue, BIN);
        //        if (serialLog) Serial.print(" REMAINING: ");
        //        if (serialLog) Serial.print(rowsScanned, BIN);
        //        if (serialLog) Serial.print(" COL VALUE: ");
        //        if (serialLog) Serial.println(scannedColValue, BIN);

      }

      //3ms is approx the time that long inhibition signal is active so no point in sampling again now
      delay(3);
    }

    if (numRowsScanned > 50) {
      if (serialLog) Serial.print("TOO MANY ROWS SCANNED: ");
      if (serialLog) Serial.println(numRowsScanned, DEC);
    }
  }

  //Code to debug board position over serial
  //  for (int i = 0; i < 8; i++) {
  //    for (int j = 0; j < 8; j++) {
  //      if (serialLog) Serial.print(scannedpos[(8 * i) + j ],  DEC);
  //    }
  //    if (serialLog) Serial.println("");
  //  }
  //
  //  if (serialLog) Serial.print("ROWS SCANNED: ");
  //  if (serialLog) Serial.println(numRowsScanned, DEC);
  //
  //  delay (500);




  //Read board state actively driving signals through the bus if on ACTIVE mode
  if (!isPassiveMode) {


    //todefault
    //LED_WR_DISABLE_COLUMN_LOAD  29
    //LED_WR_LOAD                 32
    //ROW_LOAD                    31
    //COLUMN_DISABLE              30


    //if (serialLog) Serial.println("------------->SCANNING ROWS");
    //try to latch fixed value to leds
    busValuesToDefault();
    digitalWrite(CONTROLLER_OUTPUT_DISABLE, false); //enable PIC to bus


    //Load zero in leds latch
    SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, true); //unlatch leds
    SPIExpander.writePort(1, 0b00000000);
    SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, false); //latch leds
    digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus

    int activatedRow = 0;
    //loop for each row



    for (int activatedRow = 0; activatedRow < 8; activatedRow++) {
      //Configure bus for writing to row latch
      SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, false); //latch leds
      SPIExpanderButtons.digitalWrite(COLUMN_DISABLE_EX, true); //disable column to bus
      SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, false); //disable led output  
      SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, true); //open row load latch
      digitalWrite(CONTROLLER_OUTPUT_DISABLE, false); //enable PIC to bus

      //Write row address to expander
      byte encodedRow = 0xFF;
      bitWrite(encodedRow, 7 - activatedRow, 0);
      SPIExpander.writePort(1, encodedRow);
      SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, false); //latch row
      digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus
      SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, true); //open row latch
      SPIExpanderButtons.digitalWrite(COLUMN_DISABLE_EX, false); //enable column to bus
      SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, false); //close row latch


      //read columns values
      byte  inputRead = SPIExpander.readPort(0);

      if (!boardInverted) {
        //reverse scan positions
        scannedpos[8 * activatedRow + 0 ] = !bitRead(inputRead, 0);
        scannedpos[8 * activatedRow + 1 ] = !bitRead(inputRead, 1);
        scannedpos[8 * activatedRow + 2 ] = !bitRead(inputRead, 2);
        scannedpos[8 * activatedRow + 3 ] = !bitRead(inputRead, 3);
        scannedpos[8 * activatedRow + 4 ] = !bitRead(inputRead, 4);
        scannedpos[8 * activatedRow + 5 ] = !bitRead(inputRead, 5);
        scannedpos[8 * activatedRow + 6 ] = !bitRead(inputRead, 6);
        scannedpos[8 * activatedRow + 7 ] = !bitRead(inputRead, 7);

      } else {
        scannedpos[63 - (8 * activatedRow + 0) ] = !bitRead(inputRead, 0);
        scannedpos[63 - (8 * activatedRow + 1) ] = !bitRead(inputRead, 1);
        scannedpos[63 - (8 * activatedRow + 2) ] = !bitRead(inputRead, 2);
        scannedpos[63 - (8 * activatedRow + 3) ] = !bitRead(inputRead, 3);
        scannedpos[63 - (8 * activatedRow + 4) ] = !bitRead(inputRead, 4);
        scannedpos[63 - (8 * activatedRow + 5) ] = !bitRead(inputRead, 5);
        scannedpos[63 - (8 * activatedRow + 6) ] = !bitRead(inputRead, 6);
        scannedpos[63 - (8 * activatedRow + 7) ] = !bitRead(inputRead, 7);
      }

    }

  }


  //Display leds
  displayLedRow();



  //Start of board movement detection comparing new scanned board positions with previous ones

  //Check if inverting board lifting both kings in the initial position
  if (! boardInverted && boardInInitialPos && !scannedpos[59] && !scannedpos[3] ) {

    if (serialLog) Serial.println("REVERSING BOARD");

    boardInverted = true;
    for (int j = 0; j <= 63; j++) {
      transposescannedpos[63 - j] = scannedpos[j];
    }

    for (int j = 0; j <= 63; j++) {
      scannedpos[j] = transposescannedpos[j];
    }

    for (int j = 0; j <= 63; j++) {
      transposescannedpos[63 - j] = lastpos[j];
    }

    for (int j = 0; j <= 63; j++) {
      lastpos[j] = transposescannedpos[j];
    }


    currentPos[60] = initpos[60];
    currentPos[4] = initpos[4];
  } else

    if (boardInverted && boardInInitialPos && !scannedpos[59] && !scannedpos[3] ) {

      if (serialLog) Serial.println("UNREVERSING BOARD");

      boardInverted = false;
      for (int j = 0; j <= 63; j++) {
        transposescannedpos[63 - j] = scannedpos[j];
      }

      for (int j = 0; j <= 63; j++) {
        scannedpos[j] = transposescannedpos[j];
      }

      for (int j = 0; j <= 63; j++) {
        transposescannedpos[63 - j] = lastpos[j];
      }

      for (int j = 0; j <= 63; j++) {
        lastpos[j] = transposescannedpos[j];
      }


      currentPos[60] = initpos[60];
      currentPos[4] = initpos[4];
    }

  //detect changes between lastpos and scannedpos
  byte numLifted = 0;
  byte numPositioned = 0;
  byte lastLifted = 0;
  byte lastLiftedSquare = 0;
  byte previouslyLifted = 0;
  byte previouslyLiftedSquare = 0;
  byte lastPositionedSquare = 0;
  for (int i = 0; i < 64; i++) {
    if (scannedpos[i] == true && lastpos[i] == false) {

      if (isModeALeds && modeAautoLedsOff) {
        shitchOffLed(i);
      }


      boardInInitialPos = false;
      numPositioned++;
      lastPositionedSquare = i;
      //if (serialLog) Serial.print("Positioned: ");
      //if (serialLog) Serial.println(i, DEC);
    }
    else if (scannedpos[i] == false && lastpos[i] == true) {
      //Lifted piece

      if (isModeALeds && modeAautoLedsOff) {
        shitchOffLed(i);
      }

      //Check if lifting kings in intial position to invert board
      if (//!boardInverted &&
        boardInInitialPos && (i == 59 || i == 3 || i == 60 || i == 4 )) {
        //Lifting king in intial pos, maybe want to invert board
        break;
        
      } else {

        //No longer in initial position
        boardInInitialPos = false;

      }


      //Check if we are lifting the last positioned piece. In that case we may be taking back the movement
      if (lastLifted != 0) {
        previouslyLifted = lastLifted;
        previouslyLiftedSquare = lastLiftedSquare;
      }

      //Normal lift, just send the movement to the host
      //Save status update for host
      changes[numChanges].square = i;
      changes[numChanges].oldPiece = currentPos[i];
      changes[numChanges++].newPiece = 0;
      numLifted++;
      lastLifted = currentPos[i];
      lastLiftedSquare = i;
      currentPos[i] = 0;
      //if (serialLog) Serial.print("Lifted: ");
      //if (serialLog) Serial.println(i, DEC);

      //Special case, lifting a white pawn at row 8 or black pawn at row 1 become a queen by default
      //we also enable promotion mode, where any other piece positioning doesn't remove the lastLifted value until
      //we place the promoted piece back

      //promoted piece

      if (lastLifted == 0x01 && i <= 7) {
        lastLifted = 0x06;
        promotingSquare = i;
      }
      if (lastLifted == 0x07 && i >= 56) {
        lastLifted = 0x0C;
        promotingSquare = i;
      }
      //}
    }
    lastpos[i] = scannedpos[i];
  }

  //Beeps and Boops
  if (numLifted) {
    boop();
  }

  if (numPositioned) {
    beep();
  }

  if (numLifted == 1 ) {

    if (serialLog) Serial.print("LIFTED: ");
    if (serialLog) Serial.print(liftedPiece, DEC);
    if (serialLog) Serial.print(" ");
    if (serialLog) Serial.println(lastLifted, DEC);
    if (lastMovement != NULL) {
      if (serialLog) Serial.print("LMDE: ");
      if (serialLog) Serial.print(char(lastMovement->destinationSquare % 8 + 0x41));
      if (serialLog) Serial.print(8 - (lastMovement->destinationSquare / 8));
      if (serialLog) Serial.print(" ");
      if (serialLog) Serial.print(char(lastLiftedSquare % 8 + 0x41));
      if (serialLog) Serial.print(8 - (lastLiftedSquare / 8));
    }

    //One piece lifted
    if (liftedPiece == 0) {
      //First lifted
      liftedPiece = lastLifted;
      liftedPieceSquare = lastLiftedSquare;

      //Maybe taking back...
      if (lastMovement != NULL && lastMovement->destinationSquare == liftedPieceSquare) {
        takingBackMovement = true;
        if (serialLog) Serial.println("Maybe taking back");
      }
    } else if (liftedPiece2 == 0) {
      liftedPiece2 = lastLifted;
      liftedPiece2Square = lastLiftedSquare;
      if (serialLog) Serial.println("Second lifted");
    } else {
      if (serialLog) Serial.println("LOST TRACK");
      //lost track
      //signal out-of sync to the user and ask to compose the position scanning the missing pieces
      //TBD
      boardIsOutOfSync = true;
      liftedPiece2 = 0;
      liftedPiece = lastLifted;
      liftedPieceSquare = lastLiftedSquare;
    }
  }

  else if (numLifted >= 2) {
    //two pieces lifted, maybe we lost track
    if (numLifted > 2) {
      //lost track
      //signal out-of sync to the user and ask to compose the position scanning the missing pieces
      //TBD
      boardIsOutOfSync = true;
    } else if (liftedPiece != 0 || liftedPiece2 != 0) {
      //lost track
      //signal out-of sync to the user and ask to compose the position scanning the missing pieces
      //TBD, possible future improvement
      boardIsOutOfSync = true;
    }
    liftedPiece = previouslyLifted;
    liftedPieceSquare = previouslyLiftedSquare;
    liftedPiece2 = lastLifted;
    liftedPiece2Square = lastLiftedSquare;
  }



  //If more than one piece is positioned, or one piece is positioned when no piece was raised we have lost track of the game
  //Unless we are sertting up the initial position, in that case we know what the pieces are
  if (firstScan || settingUpBoardMode ||  (numPositioned > 0 && liftedPiece == 0)) {


    //check if there are pieces only in the starting rows
    boolean allPiecesInStartingRows = true;
    int unknownPiecesInStartingRows = 0;
    for (int i = 0; i <= 63; i++) {
      if (scannedpos[i] && ! initpos[i]) {
        allPiecesInStartingRows = false;
        if (settingUpBoardMode) {
          settingUpBoardMode = false;
          if (serialLog) Serial.println("Disabling setting up board mode");
        }
      }

      if (initpos[i] && scannedpos[i] && !currentPos[i]) {
        unknownPiecesInStartingRows++;
      }
    }

    //Set missing pieces only in the starting rows if more than 4 are unknown
    if (firstScan || (allPiecesInStartingRows && (unknownPiecesInStartingRows >= 4 || settingUpBoardMode))) {

      if (!firstScan && !settingUpBoardMode) {
        if (serialLog) Serial.println("Enabling setting up board mode");
        settingUpBoardMode = true;
      }

      for (int i = 0; i <= 63; i++) {
        if (scannedpos[i] && currentPos[i] != initpos[i]) {
          currentPos[i] = initpos[i];
          if (serialLog) Serial.print("Restoring piece at: ");
          if (serialLog) Serial.println(i, DEC);
        }
      }
    } else {
      //signal out-of sync to the user and ask to compose the position scanning the missing pieces
      //TBD, possible future improvement
      boardIsOutOfSync = true;
    }
  }




  //try to guess positioned piece value
  if (numPositioned == 1 && liftedPiece != 0) {

    boolean takenBack = false;

    //If we are taking back a movement the piece has to be positioned in the same square to abort or
    //in the previous square to take back
    //In the latter case a taken piece may need to be positioned back
    if (takingBackMovement) {
      //look at previous move to read origin and destination squares
      if (lastMovement != NULL) {
        if (lastMovement->originSquare ==  lastPositionedSquare) {

          takenBack = true;

          liftedPiece = lastMovement->originPiece;

          if (Serial) {
            if (serialLog) Serial.print("TAKING BACK: ");
            if (serialLog) Serial.print(liftedPiece, DEC);
            if (serialLog) Serial.print(" ");
            if (serialLog) Serial.print(char(lastMovement->destinationSquare  % 8 + 0x41));
            if (serialLog) Serial.print(8 - (lastMovement->destinationSquare / 8));
            if (serialLog) Serial.print(" ");
            if (serialLog) Serial.print(lastMovement->originPiece, DEC);
            if (serialLog) Serial.print(" ");
            if (serialLog) Serial.print(char(lastPositionedSquare % 8 + 0x41));
            if (serialLog) Serial.print(8 - (lastPositionedSquare / 8));
            if (serialLog) Serial.print(" ");

          }
          //Maybe origin piece is different...
          currentPos[lastPositionedSquare] =  lastMovement->originPiece;

          //Taking back, if some piece was taking move it to its square
          if (lastMovement->takenPiece) {

            if (serialLog) Serial.print(lastMovement->takenPiece, DEC);
            if (serialLog) Serial.print(" ");
            if (serialLog) Serial.print(char(lastMovement->takenPieceSquare % 8 + 0x41));
            if (serialLog) Serial.print(8 - (lastMovement->takenPieceSquare / 8));

            //Make it lifted
            //liftedPiece2 = lastMovement->takenPiece;
            //position taken piece
            currentPos[lastMovement->takenPieceSquare] =  lastMovement->takenPiece;
            //signal out-of sync to the user and ask to compose the position scanning the missing pieces
            //TBD, possible future improvement
            boardIsOutOfSync = true;
          }
          //une movement back
          Movement *deleteMovement = lastMovement;
          lastMovement = lastMovement->previousMovement;
          delete deleteMovement;

          if (serialLog) Serial.println("");
        }

        else if (lastMovement->destinationSquare ==  lastPositionedSquare) {
          //Aborting take back, do nothing.

        } else {
          //moved to a completely different square, just changing the movement or something weird
          lastMovement->destinationSquare = lastPositionedSquare;
        }

      } else {
        //No movements, wtf??, why are we taking back if no movements?
      }

    }

    if (!takenBack) {

      //If two pieces were raised, the positioned piece is the piece from the other square
      //unless one pawn is changing column, in that case it is en-passant taking
      //register movement if completed
      if (liftedPiece2 != 0) {
        //Piece taken, try to guess which is taken or en-passant taken
        if (lastPositionedSquare == liftedPiece2Square) {
          //Moved  liftedPiece, register movement
          registerMovement(liftedPiece, liftedPieceSquare, liftedPiece, lastPositionedSquare, liftedPiece2, liftedPiece2Square );

        } else if (lastPositionedSquare == liftedPieceSquare) {
          //Moved  liftedPiece2, register movement
          registerMovement(liftedPiece2, liftedPiece2Square, liftedPiece2, lastPositionedSquare, liftedPiece, liftedPieceSquare );
          liftedPiece = liftedPiece2;

        } else if ((liftedPiece == 1 || liftedPiece == 7) && (liftedPiece2 == 1 || liftedPiece2 == 7)  && (liftedPiece2Square % 8 == lastPositionedSquare % 8)) {
          //Moved en-passant liftedPiece, register
          if (serialLog) Serial.println("EP1");
          registerMovement(liftedPiece, liftedPieceSquare, liftedPiece, lastPositionedSquare, liftedPiece2, liftedPiece2Square );

        } else if ((liftedPiece == 1 || liftedPiece == 7) && (liftedPiece2 == 1 || liftedPiece2 == 7)  && (liftedPieceSquare % 8 == lastPositionedSquare % 8)) {
          //Moved en-passant liftedPiece2, register
          if (serialLog) Serial.println("EP2");
          registerMovement(liftedPiece2, liftedPiece2Square, liftedPiece2, lastPositionedSquare, liftedPiece, liftedPieceSquare );
          liftedPiece = liftedPiece2;

        } else {
          if (serialLog) Serial.println("xxxxxxxxxxxxxxxxxxNFI");
          //NFI
        }
      }

      //If only one piece raised can still be en-passant, in that case force remove the taken pawn and declare out of sync
      else if ((liftedPiece == 1 || liftedPiece == 7)  && (liftedPieceSquare % 8 != lastPositionedSquare % 8)) {
        //Moved en-passant liftedPiece, register
        //remove taken pawn
        //if (!boardInverted) {
        if (liftedPiece == 1 && lastPositionedSquare <= (63 - 8)) {
          if (serialLog) Serial.println("EP3");
          //white
          registerMovement(liftedPiece, liftedPieceSquare, liftedPiece, lastPositionedSquare, 7, lastPositionedSquare + 8 );
          currentPos[lastPositionedSquare + 8] = 0;
          //Save status update for pawn removal for host
          changes[numChanges].square = lastPositionedSquare + 8;
          changes[numChanges].oldPiece = 7;
          changes[numChanges++].newPiece = 0;


        }  else if (liftedPiece == 7 && lastPositionedSquare >= 8 ) {
          //black
          if (serialLog) Serial.println("EP4");
          registerMovement(liftedPiece, liftedPieceSquare, liftedPiece, lastPositionedSquare, 1, lastPositionedSquare - 8 );
          currentPos[lastPositionedSquare - 8] = 0;
          //Save status update for pawn removal for host
          changes[numChanges].square = lastPositionedSquare - 8;
          changes[numChanges].oldPiece = 1;
          changes[numChanges++].newPiece = 0;
        }

        boardIsOutOfSync = true;
      }

      else {
        //Register regular move, or not move at all
        if (liftedPieceSquare != lastPositionedSquare) {
          registerMovement(liftedPiece, liftedPieceSquare, liftedPiece, lastPositionedSquare, 0, 0 );
        }
      }
    }

    //Save status update for host
    currentPos[lastPositionedSquare] = liftedPiece;
    changes[numChanges].square = lastPositionedSquare;
    changes[numChanges].oldPiece = 0;
    changes[numChanges++].newPiece = liftedPiece;


    //Check if we are placing back a promoted pawn
    if (promotingSquare <= 63) {
      if (lastPositionedSquare == promotingSquare) {
        promotingSquare = 100; //Disable
        //edit last movement
        lastMovement->destinationPiece = liftedPiece;
        liftedPiece = 0;
        liftedPiece2 = 0;

      }
    } else {
      liftedPiece = 0;
      liftedPiece2 = 0;
    }

    //Already processed
    takingBackMovement = false;
  }




  //Check if we are in the starting position or in the "play black" starting position

  //we check also if it is just missing one piece. In that case enter RFID programming mode in case we want to program that piece
  int missingPiece = 0;
  boolean sendUpdate = false;
  for (int i = 0; i <= 63; i++) {
    if ((initpos[i] && scannedpos[i] ) || (!initpos[i] && ! scannedpos[i])) {

      if (i == 63 && ! missingPiece ) {
        //Starting pos white
        for (int j = 0; j <= 63; j++) {
          if (currentPos[j] != initpos[j]) {
            sendUpdate = true;
          }
          currentPos[j] = initpos[j];
        }
        //memcpy(&currentPos, &initpos, sizeof(initpos[0]) * 64);
        //}

        promotingSquare = 100;
        liftedPiece = 0;
        liftedPiece2 = 0;
        boardInInitialPos = true;

        if (sendFullUpdateOnNextInitialPos) {
          sendFullUpdateOnNextInitialPos = false;
          if (!firstCharacterReceived && !isTypeBUSBBoard) {
            sendBoardDump();
            if (serialLog) Serial.println("Sent full board update on mode A board");
          } else {
            sendStatusIfNeeded();
            if (serialLog) Serial.println("Sent full board update on mode B board");
          }
        }

        while (lastMovement != NULL) {
          Movement *deleteMovement = lastMovement;
          lastMovement = lastMovement->previousMovement;
          delete deleteMovement;
        }
      } else if (i == 63) {
        //Not initial position, rearm sending full update once when initial position
        sendFullUpdateOnNextInitialPos = true;
      }
      continue;
    } else if (! missingPiece && initpos[i] && ! scannedpos[i] ) {
      missingPiece = initpos[i] ;

    } else {
      missingPiece = 99;
    }
  }


  firstScan = false;



  //if (serialLog) Serial.println("LIA");


  //We enter program mode to write to RFID tag if we are in the starting position minus one piece, so that we know which piece has been lifted
  if (missingPiece && missingPiece < 99) {

    if (programRfidTag(missingPiece)) {
      //Successfully programmed
      boop();
      beep();
      boop();
    }
  }

  //BLE status update
  long currentTime = millis();

  if (updateModeTypeBUSBBoard == 0 && currentTime > (lastUpdateTimeTypeBUSBBoard +  40 )) {
    //Update every scan
    sendStatusIfNeeded();

  } else if (updateModeTypeBUSBBoard == 2 && currentTime > (lastUpdateTimeTypeBUSBBoard +  updateIntervalTypeBUSBBoard ))  {
    //Timed update, update only if enough time has passed since last update
    sendStatusIfNeeded();

  } else if (updateModeTypeBUSBBoard >= 3 && (numChanges > 0 || sendUpdate) ) {
    //Update on change
    sendStatusIfNeeded();
  }

  lastUpdateTimeTypeBUSBBoard = currentTime;

  //if (serialLog) Serial.println("LI0");

  //Data back to Mode A driver
  if (!firstCharacterReceived && !isTypeBUSBBoard) {
    sendPendingUpdates();
  } else {
    numChanges = 0;
  }

  //Lichess mode loop
  if (isLichessConnected) {

    if (isLichessGameActive()) {
      getIncomingGameEventsLichess();
      fixBoardPositionLichessData();
      setLichessBoardLeds();
      refreshLichessClocks();
    } else {
      getLichessTvEvents();
      clearAllBoardLeds();
      refreshLichessClocks();
    }


    refreshLichessPlayerTopSprite();
    refreshLichessPlayerBottomSprite();
  }


  //Refresh display
  displayBoard();

  //Read module buttons
  read_module_buttons();

  //Search for NFC tags to identify lifted pieces
  //Ignore if we are in programming mode
  if (!(missingPiece && missingPiece < 99)) {

    if (readRfidTag(&liftedPiece)) {
      boop();
    }
  }


  //We block here if there are pending external clock messages to the host
  if (isExternalClock) {
    checkForPendingExternalClockMessages();
  }

  //END OF MAIN LOOP

}


//Send tick data from clock to host
void processPendingExternalClockMessageTime(int lhours_p, int  lmins_p, int  lsecs_p, int  rhours_p, int  rmins_p, int  rsecs_p, int  leverState, int  updt) {


  rhours = bcdToDec(rhours_p);
  rmins = bcdToDec(rmins_p);
  rsecs = bcdToDec(rsecs_p);
  lhours = bcdToDec(lhours_p);
  lmins = bcdToDec(lmins_p);
  lsecs = bcdToDec(lsecs_p);


  if (serialLog) Serial.print("SENDING EXTERNAL CLOCK DATA. UPDATE MODE: ");
  if (serialLog) Serial.println(updateMode, DEC);


  leverPositionHighRight = leverState;

  if (serialLog) Serial.print("Lever state right high: ");
  if (serialLog) Serial.println(leverPositionHighRight, DEC);


  if (updateMode != 0) {
    send_clock();
  }

}


void processPendingExternalClockMessageButtonChange(int button) {
  if (serialLog) Serial.print("SENDING EXTERNAL CLOCK BUTTON PUSH COMMAND: ");
  if (serialLog) Serial.println(button, DEC);
  send_button_push(button);
}




void registerMovement(byte originPiece, byte originSquare, byte destinationPiece, byte destinationSquare, byte takenPiece, byte takenPieceSquare ) {


  Movement *movement = new Movement;
  movement->originPiece = originPiece;
  movement->originSquare = originSquare;
  movement->destinationPiece = destinationPiece;
  movement->destinationSquare = destinationSquare;
  movement->takenPiece = takenPiece;
  movement->takenPieceSquare = takenPieceSquare;
  movement->previousMovement = lastMovement;
  if (lastMovement != NULL) {
    lastMovement->nextMovement = movement;
  }
  lastMovement = movement;
  movement->nextMovement = NULL;




  if (serialLog) Serial.print("Register: ");
  if (serialLog) Serial.print(originPiece, DEC);
  if (serialLog) Serial.print(" ");
  if (serialLog) Serial.print(char(originSquare % 8 + 0x41));
  if (serialLog) Serial.print(8 - (originSquare / 8));
  if (serialLog) Serial.print(" ");
  if (serialLog) Serial.print(destinationPiece, DEC);
  if (serialLog) Serial.print(" ");
  if (serialLog) Serial.print(char(destinationSquare % 8 + 0x41));
  if (serialLog) Serial.print(8 - (destinationSquare / 8));
  if (serialLog) Serial.print(" ");
  if (serialLog) Serial.print(takenPiece, DEC);
  if (serialLog) Serial.print(" ");
  if (serialLog) Serial.print(char(takenPieceSquare % 8 + 0x41));
  if (serialLog) Serial.println(8 - (takenPieceSquare / 8));


  //If in lichess mode send the movement if we are in our turn
  if (isLichessGameActive() && isMyTurnLichess() ) {
    //Detect castling TBD
    if (serialLog) Serial.print("Sending movement...");
    while (! sendOutgoingMovementLichess(originSquare, destinationSquare)) {
      if (serialLog) Serial.print("Retry sending movement...");
    }
  }

}
