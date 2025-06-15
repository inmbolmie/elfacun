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
#include "ble_board_c.h"
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
#include "esp_bt_device.h"


#define INCLUDE_vTaskSuspend


// These are used to get information about static SRAM and flash memory sizes
extern "C" char __data_start[];    // start of SRAM data
extern "C" char _end[];     // end of SRAM data (used to check amount of SRAM this program's variables use)
extern "C" char __data_load_end[];  // end of FLASH (used to check amount of Flash this program's code and data uses)

int numloop = 0;

boolean firstScan = true;

boolean evenScan = true;

boolean chess960ModeAlreadySetUp = true; 

boolean positionRestoredFromHoldMode = false;

boolean startLichessConnection = false;

unsigned long timeScanRfid = 0;
unsigned long rfid_scan_interval = 250l;
unsigned long timeUpdateSprites = 0;
unsigned long sprite_update_interval = 50l;
unsigned long timeScan = 0;
unsigned long scan_interval = 25l;
unsigned long timeUpdateButtons = 0;
unsigned long buttons_update_interval = 50l;



//Hardware initialization
void setup() {

  ledbuffer.reserve(167);

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
  //LATCH_ROWS_DISABLE is output in V1 and WRITE_ENABLE_SAFE input in V2 on the same pin, so it is safer as input by default unless we detect V1 hardware
  pinMode(WRITE_ENABLE_SAFE, INPUT);
  pinMode(LATCH_COLUMNS_DISABLE, OUTPUT);
  //digitalWrite(LATCH_ROWS_DISABLE, true);
  //digitalWrite(LATCH_COLUMNS_DISABLE, true);

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

  SPI.setFrequency(10000000);

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
  //This will be switched to input when hardware detection is finished
  SPIExpanderButtons.pinMode(HARDWARE_V2_DETECT, INPUT_PULLUP);
  //This will be switched to output if V2 hardware is detected
  SPIExpanderButtons.pinMode(7, INPUT);

  //BUS CONTROL SIGNALS
  SPIExpanderButtons.pinMode(8, OUTPUT);
  SPIExpanderButtons.pinMode(9, OUTPUT);
  SPIExpanderButtons.pinMode(10, OUTPUT);
  SPIExpanderButtons.pinMode(11, OUTPUT);
  //These 4 will be switched to outputs if V2 hardware is detected
  SPIExpanderButtons.pinMode(12, INPUT);
  SPIExpanderButtons.pinMode(13, INPUT);
  SPIExpanderButtons.pinMode(14, INPUT);
  SPIExpanderButtons.pinMode(15, INPUT);

  //Put the bus in staeady mode
  busValuesToDefault();


  //Detect V2 hardware with improved passive mode capture hardware
  isV2Hardware = !SPIExpanderButtons.digitalRead(HARDWARE_V2_DETECT);

  if (!isV2Hardware) {
    //disable passive mode latches
    pinMode(LATCH_ROWS_DISABLE, OUTPUT);
    pinMode(LATCH_COLUMNS_DISABLE, OUTPUT);
    digitalWrite(LATCH_ROWS_DISABLE, true);
    digitalWrite(LATCH_COLUMNS_DISABLE, true);

  } else {
    //Set brightness to a medium value
    pinMode(SCREEN_BRIGHTNESS, OUTPUT);
    setTftBrightness(64);
  }

  SPIExpanderButtons.pinMode(HARDWARE_V2_DETECT, INPUT);

  //V2 hardware initialization
  if (isV2Hardware) {

    pinMode(SCREEN_BRIGHTNESS, OUTPUT);

    SPIExpanderButtons.pinMode(MEM_WRITE_ENABLE, OUTPUT);
    SPIExpanderButtons.pinMode(MEM_ADDR_0, OUTPUT);
    SPIExpanderButtons.pinMode(MEM_ADDR_1, OUTPUT);
    SPIExpanderButtons.pinMode(MEM_ADDR_2, OUTPUT);
    SPIExpanderButtons.pinMode(MEM_ADDR_SRC, OUTPUT);

    SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, false);
    SPIExpanderButtons.digitalWrite(MEM_ADDR_0, false);
    SPIExpanderButtons.digitalWrite(MEM_ADDR_1, false);
    SPIExpanderButtons.digitalWrite(MEM_ADDR_2, false);
    SPIExpanderButtons.digitalWrite(MEM_ADDR_SRC, MEM_ADDR_SRC_BUS);

  }


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

    checkRemoteUpdateFromFS(SD);

    //Check for new resources
    //Commented out as it is somehow corrupting SPIFFS
    //SPIFFS.begin();
    //load_file_from_sd_to_spiffs("/modea.bmp");
    //load_file_from_sd_to_spiffs("/modeb.bmp");
    //load_file_from_sd_to_spiffs("/modec.bmp");
    //clearScreen();
    //SPIFFS.end();

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

  //Initialize SPIFFS to get bmp files for displaying
  SPIFFS.begin();


  //Clear screen
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  //tft.setTextColor(TFT_WHITE); tft.setTextSize(1);

  //Read config from NVS if has been stored there at any time from SD card
  read_config_nvs();





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
    tft.println("Setting default mac addr");
  }



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

  boolean forcePassiveMode = !SPIExpanderButtons.digitalRead(3);

  //dim screen to reduce battery draw while initializing
  setTftBrightness(0);

  boolean startBleService = false;

  boolean startBleServiceC = false;

  enableNvsLog = false;

  centerBoardOnScreen = false;

  boolean lichessGameStarted = isLichessGameInProgress();

  if ((noButtonsPushed && strcmp(defaultMode, "L") == 0) || !SPIExpanderButtons.digitalRead(2) || (noButtonsPushed && lichessGameStarted) ) {
    //For Lichess

    isBrightnessAdjustmentDisabled = false;

    displayLichessLogo();

    initializeWifi();

    delay(2000);

    tft.setCursor(0, 0);

    Serial.begin(115200);

    if (isV2Hardware) {
      if (serialLog) Serial.println("V2 hardware lichess mode");
    } else {
      if (serialLog) Serial.println("V1 hardware lichess mode");
    }

    startLichessConnection = true;

    //Disable board scanning until we process the first fen
    disableBoardScanning = true;

  }


  else if ((noButtonsPushed && strcmp(defaultMode, "C") == 0) || (!SPIExpanderButtons.digitalRead(1) && !SPIExpanderButtons.digitalRead(0))) {
    //For type C board

    isBrightnessAdjustmentDisabled = false;

    if (!noButtonsPushed && lichessGameStarted) {
      //abort
      setLichessGameFinished();
    }

    displayModeCLogo();

    Serial.begin(9600);
    if (isV2Hardware) {
      if (serialLog) Serial.println("V2 hardware C mode");
    } else {
      if (serialLog) Serial.println("V1 hardware C mode");
    }

    //Needed to read the mac and not confuse IOS devices with the new mac
    SerialBT.begin(modeABtAdvertisedName);

    //Change mac address for mode C
    const uint8_t* mac;


    mac = esp_bt_dev_get_address();

    if (mac[5] <= 250) {
      modeCmac[5] = mac[5] + 2;
    } else {
      modeCmac[5] = mac[5] - 6;
  }

    modeCmac[4] = mac[4];
    modeCmac[3] = mac[3];
    modeCmac[2] = mac[2];
    modeCmac[1] = mac[1];
    modeCmac[0] = mac[0];

    esp_base_mac_addr_set(modeCmac);

    SerialBT.end();
    SerialBT.begin(modeCBtAdvertisedName);

    initBleC();

    startBleServiceC = true;


  }






  else if ((noButtonsPushed && strcmp(defaultMode, "B") == 0) || !SPIExpanderButtons.digitalRead(1)) {
    //For type B board

    isBrightnessAdjustmentDisabled = false;

    if (!noButtonsPushed && lichessGameStarted) {
      //abort
      setLichessGameFinished();
    }

    Serial.begin(38400, SERIAL_7O1);
    if (isV2Hardware) {
      if (serialLog) Serial.println("V2 hardware B mode");
    } else {
      if (serialLog) Serial.println("V1 hardware B mode");
    }

    displayModeBLogo();

    SerialBT.begin(modeBBtAdvertisedName);
    isTypeBUSBBoard = true;

    centerBoardOnScreen = true;

    initBle();

    startBleService = true;

  } else {

    isBrightnessAdjustmentDisabled = false;

    if (!noButtonsPushed && lichessGameStarted) {
      //abort
      setLichessGameFinished();
    }

    //For type A board
    displayModeALogo();

    Serial.begin(9600);
    if (isV2Hardware) {
      if (serialLog) Serial.println("V2 hardware A mode");
    } else {
      if (serialLog) Serial.println("V1 hardware A mode");
    }

    SerialBT.begin(modeABtAdvertisedName);

    initBle();

    startBleService = true;

  }


  //initialize RFID reader/writer
  rfid_init();

  if (startLichessConnection) {
    rfid_scan_interval = 1000l;
    scan_interval = 50l;
  }


  //Sound will be disabled if we boot pushing the change lever button
  if ((!SPIExpanderButtons.digitalRead(5) && (strcmp(buzzerEnabled, "TRUE") == 0)) || (SPIExpanderButtons.digitalRead(5) && (strcmp(buzzerEnabled, "FALSE") == 0))) {
    if (serialLog) Serial.println("Disabling sound from button");
    soundIsDisabled = true;
  }


  if (!SPIExpanderButtons.digitalRead(5) && (strcmp(buzzerEnabled, "FALSE") == 0)) {
    if (serialLog) Serial.println("Enabling sound from button");
    soundIsDisabled = false;
  }


  //Leds will be disabled if we boot pushing the next button
  if (!SPIExpanderButtons.digitalRead(4) && (strcmp(ledsEnabled, "TRUE") == 0)) {
    if (serialLog) Serial.println("Disabling leds from button");
    ledsAreDisabled = true;
  }


  if (!SPIExpanderButtons.digitalRead(4) && (strcmp(ledsEnabled, "FALSE") == 0)) {
    if (serialLog) Serial.println("Enabling leds from button");
    ledsAreDisabled = false;
  }

  //LOG CPU SPEED
  if (serialLog) Serial.print("ESP32 CPU SPEED: ");
  if (serialLog) Serial.println(rtc_clk_cpu_freq_value(rtc_clk_cpu_freq_get()), DEC);




  //Setup board initial position with zeroes
  memset(&currentPos, 0, sizeof(initpos[0]) * 64);
  promotingSquare = 100;
  boardInInitialPos = false;

  if (!lichessGameStarted) {
    boardInverted = false;
  }

  //Recover a board position if it was stored on NVS in hold mode
  if (recoverNVSBoardPosition()) {
    positionRestoredFromHoldMode = true;
    firstScan = false;
  }

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

  if ((strcmp(buzzerEnabledPassiveLed, "FALSE") == 0)) {
    soundIsDisabledPassiveLed = true;
  }


  if (isV2Hardware) {
    setDefaultTftBrightness();
  }

  //Look for an additional chess module plugged into the board and enter passive mode if we find one
  //To find external modules we look for activity in the bus control lines for some seconds
  //In passive mode we won't be actively managing neither board scanning nor led lighting
  //that will be controlled by the other module and we will passively listen to the signals

  //If the position is restored from hold mode the passive status was restored as well


  if (forcePassiveMode) {
    isPassiveMode = true;
  } else  {
    if (!positionRestoredFromHoldMode) {
  isPassiveMode = checkForExternalModule();
    }
  }

  if (startBleService) {
    //Try to connect to LED service on V2 modules passive mode
    if (isV2Hardware && isPassiveMode && (strcmp(dualLedsEnabled, "TRUE") == 0)) {
      connectToServer();
    }
    initBleService();
  }

  if (startBleServiceC) {
    //Try to connect to LED service on V2 modules passive mode
    if (isV2Hardware && isPassiveMode && (strcmp(dualLedsEnabled, "TRUE") == 0)) {
      connectToServer();
    }
    initBleServiceC();
  }



  if (isPassiveMode && !isV2Hardware) {
    //Passive mode on V1 hardware. An original module will control the leds and board scanning and we will listen passively and capture the data
    if (serialLog) Serial.println("Entering PASSIVE mode V1 hardware");



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

    //ENABLE BOARD BUS
    pinMode(EN_BUS, OUTPUT);
    digitalWrite(EN_BUS, true);


  } else if (isPassiveMode && isV2Hardware) {

    //Passive mode on V2 hardware. An original module will control the leds and board scanning and we will listen passively and capture the data

    if (serialLog) Serial.println("Entering PASSIVE mode V2 hardware");

    SPIExpanderButtons.pinMode(LED_WR_DISABLE_COLUMN_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(LED_WR_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(ROW_LOAD_EX, INPUT);
    SPIExpanderButtons.pinMode(COLUMN_DISABLE_EX, INPUT);

    //Data pins are always INPUT
    digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus


    //REVERSE OUTPUT PORT TO READ PASSIVE MODE MEMORY
    SPIExpander.pinMode(8, INPUT);
    SPIExpander.pinMode(9, INPUT);
    SPIExpander.pinMode(10, INPUT);
    SPIExpander.pinMode(11, INPUT);
    SPIExpander.pinMode(12, INPUT);
    SPIExpander.pinMode(13, INPUT);
    SPIExpander.pinMode(14, INPUT);
    SPIExpander.pinMode(15, INPUT);

    SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, true);

    //ENABLE BOARD BUS
    pinMode(EN_BUS, OUTPUT);
    digitalWrite(EN_BUS, true);

    //give time to the board position to update for the first time
    delay(500);

  } else {

    //Active mode, no other module is present so we control the leds and board scanning actively
    //23S17 ports back to output
    if (isV2Hardware) {
      if (serialLog) Serial.println("Entering ACTIVE mode V2 hardware");
    } else {
      if (serialLog) Serial.println("Entering ACTIVE mode V1 hardware");
    }

    SPIExpanderButtons.pinMode(LED_WR_DISABLE_COLUMN_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(LED_WR_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(ROW_LOAD_EX, OUTPUT);
    SPIExpanderButtons.pinMode(COLUMN_DISABLE_EX, OUTPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_A, OUTPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_B, INPUT);
    SPIExpanderButtons.pinMode(DISPLAY_CTRL_C, OUTPUT);

    if (!isV2Hardware) {
    //FREEZE EXTERNAL DISPLAY
    SPIExpanderButtons.digitalWrite(DISPLAY_CTRL_A, false);
    SPIExpanderButtons.digitalWrite(DISPLAY_CTRL_C, false);
    }


    //ENABLE BOARD BUS
    pinMode(EN_BUS, OUTPUT);
    digitalWrite(EN_BUS, true);

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




  //Clear screen
  tft.loadFont(AA_FONT_SMALL);
  clearScreen();
  tft.setCursor(0, 0);
}







//Main program loop
void loop() {

  //if (serialLog) Serial.println("Loop");
  evenScan = !evenScan;

  //Check if Bluetooth is connected
  if (!isLichessConnected && !startLichessConnection) {
    btConnected = SerialBT.connected(10);
  }

  if ((!isLichessConnected && !startLichessConnection && SerialBT.available()) || Serial.available())
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


  //BLE remote update
  if (sendRemoteUpdateFileOverBLE) {
    performRemoteUpdate();
    sendRemoteUpdateFileOverBLE = false;
    bleDeviceRequestedUpdateFile = false;

  }

  unsigned long now = millis();
  if (now > timeScan) {

    timeScan = now + scan_interval;

    //GET SPI BUS
    boolean isSemaphoreTaken = true;
    if (!xSemaphoreTake( xScreenSemaphore, ( TickType_t ) screenSemaphoreTimeout )) {
      isSemaphoreTaken = false;
      if (serialLog) Serial.println("XXXXXXXXXXXXXXX   ERROR TAKING SEMAPHORE AT (now > timeScan), skipping loop ");

    } else {

      //START OF SPI CRITICAL SECTION

  //Read board state passively reading the bus if on PASSIVE mode. We will loop waiting for data for the eight rows that usually we will get in sequence
  int numRowsScanned = 0;
      if (!holdMode && isPassiveMode && !isV2Hardware && !disableBoardScanning) {

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
              scannedpos[ (8 * (7 - encodedRowValue)) + 0 ] = !bitRead(scannedColValue, 7);
              scannedpos[ (8 * (7 - encodedRowValue)) + 1 ] = !bitRead(scannedColValue, 6);
              scannedpos[ (8 * (7 - encodedRowValue)) + 2 ] = !bitRead(scannedColValue, 5);
              scannedpos[ (8 * (7 - encodedRowValue)) + 3 ] = !bitRead(scannedColValue, 4);
              scannedpos[ (8 * (7 - encodedRowValue)) + 4 ] = !bitRead(scannedColValue, 3);
              scannedpos[ (8 * (7 - encodedRowValue)) + 5 ] = !bitRead(scannedColValue, 2);
              scannedpos[ (8 * (7 - encodedRowValue)) + 6 ] = !bitRead(scannedColValue, 1);
              scannedpos[ (8 * (7 - encodedRowValue)) + 7 ] = !bitRead(scannedColValue, 0);
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


      if (!holdMode && isPassiveMode && isV2Hardware && evenScan && !disableBoardScanning) {


        //Read position from memory
        numScansV2++;
        //Switch memory to read mode
        SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, false);
        //wait till it is safe to scan

        unsigned long startTime = millis();
        while (digitalRead(WRITE_ENABLE_SAFE)) {
          if (millis() > startTime + 500) {
            slowMode = true;
            SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, true);
            goto SKIP;
          }
        }


        if (positionRestoredFromHoldMode) {
          //We force WRITE_ENABLE_SAFE to make at least a transition from false to true to check that there is really a module out there
          //needed not to lose the restored position
          SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, true);
          startTime = millis();
          while (!digitalRead(WRITE_ENABLE_SAFE)) {
            if (millis() > startTime + 500) {
              slowMode = true;
              goto SKIP;
            }
          }

          //There is a module out there, resume normal operation
          SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, false);
          startTime = millis();
          while (digitalRead(WRITE_ENABLE_SAFE)) {
            if (millis() > startTime + 500) {
              slowMode = true;
              SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, true);
              goto SKIP;
            }
          }

        }

        slowMode = false;

        delayMicroseconds(1);
        SPIExpanderButtons.digitalWrite(MEM_ADDR_SRC, MEM_ADDR_SRC_PIC);


        boolean piecesOnBoard = false;
        byte scannedColValue = 0;
        while (numRowsScanned < 8) {

          SPIExpanderButtons.digitalWrite(MEM_ADDR_0, numRowsScanned & 0x01);
          SPIExpanderButtons.digitalWrite(MEM_ADDR_1, (numRowsScanned & 0x02) >> 1);
          SPIExpanderButtons.digitalWrite(MEM_ADDR_2, (numRowsScanned & 0x04) >> 2);
          delayMicroseconds(1);
          scannedColValue = SPIExpander.readPort(1);

          for (int i = 0; i < 8; i++) {

            boolean scannedBit;

            int rowIndex;

            if  (!boardInverted)  {
              scannedBit = bitRead(scannedColValue, i);
              rowIndex = numRowsScanned;
            } else {
              scannedBit = bitRead(scannedColValue, (7 - i));
              rowIndex = 7 - numRowsScanned;
            }

            if (scannedBit) {
              piecesOnBoard = true;
            }

            if (scannedBit != scannedposV2[(8 * rowIndex) + i ]) {
              //begin again
              numScansV2 = 0;
            }

            scannedposV2[(8 * rowIndex) + i ] = scannedBit;

          }
          numRowsScanned++;
        }


        //Count changed squares
        int numPiecesScanned = 0;
        int numPiecesPos = 0;
        for (int i = 0; i < 64; i++) {
          if (scannedpos[i]) {
            numPiecesScanned++;
          }

          if (scannedposV2[i]) {
            numPiecesPos++;
          }
        }
        boolean moreThanOnePieceChanged = (numPiecesScanned > numPiecesPos + 1) || (numPiecesPos > numPiecesScanned + 1);


        //If we are coming from hold mode we require at least one piece to be present for a valid scan and at least MIN_SCANS_V2_HOLD repeats

        if (!positionRestoredFromHoldMode || piecesOnBoard) {

          //check for minimum repetitions to copy to scanned
          if ((!positionRestoredFromHoldMode && numScansV2 >= MIN_SCANS_V2) || (positionRestoredFromHoldMode && numScansV2 >= MIN_SCANS_V2_HOLD) ) {

            //If more than one square changed, we scan more times. When it is only one it is not that important
            //because if it is a glitch it will auto-correct when the piece is back
            if (!moreThanOnePieceChanged || (numScansV2 >= MIN_SCANS_V2_PASSIVE_MULTIPLE))  {

              positionRestoredFromHoldMode = false;
              for (int i = 0; i < 64; i++) {
                scannedpos[i] = scannedposV2[i];
              }
              numScansV2 = 0;

            }
          }
        }


        //Switch memory to write mode
        SPIExpanderButtons.digitalWrite(MEM_ADDR_SRC, MEM_ADDR_SRC_BUS);
        delayMicroseconds(1);
        SPIExpanderButtons.digitalWrite(MEM_WRITE_ENABLE, true);

      }


SKIP:;


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
      if (!holdMode && !isPassiveMode && !disableBoardScanning) {

        numScansV2++;

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
          SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, false); //led output
          SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, false); //close row load latch
      digitalWrite(CONTROLLER_OUTPUT_DISABLE, false); //enable PIC to bus

      //Write row address to expander
      byte encodedRow = 0xFF;
      bitWrite(encodedRow, 7 - activatedRow, 0);
      SPIExpander.writePort(1, encodedRow);
          SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, true); //open row load latch
          delayMicroseconds(500);
      SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, false); //latch row
      digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus
      SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, true); //open row latch
      SPIExpanderButtons.digitalWrite(COLUMN_DISABLE_EX, false); //enable column to bus
          delayMicroseconds(500);
      SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, false); //close row latch

      //read columns values
      byte  inputRead = SPIExpander.readPort(0);

          //DELETE
          /*if (inputRead != 0x00 && (activatedRow == 0 || activatedRow == 1 || activatedRow == 6 || activatedRow == 7 ) ) {
            Serial.print("GLITCH: ");
            Serial.print("ROW: ");
            Serial.print(activatedRow, HEX);
            Serial.print(" COL: ");
            Serial.println(inputRead, HEX);

            }*/

      if (!boardInverted) {
        //reverse scan positions
            for (int i = 0; i < 8; i++) {
              if (!bitRead(inputRead, i) != scannedposV2[8 * activatedRow + i ]) {
                //begin again
                numScansV2 = 0;
              }
              scannedposV2[8 * activatedRow + i ] = !bitRead(inputRead, i);
            }

      } else {

            for (int i = 0; i < 8; i++) {
              if (!bitRead(inputRead, i) != scannedposV2[63 - (8 * activatedRow + i) ]) {
                //begin again
                numScansV2 = 0;
              }
              scannedposV2[63 - (8 * activatedRow + i) ] = !bitRead(inputRead, i);
            }
          }
      }

        //Count changed squares
        int numPiecesScanned = 0;
        int numPiecesPos = 0;
        for (int i = 0; i < 64; i++) {
          if (scannedpos[i]) {
            numPiecesScanned++;
    }

          if (scannedposV2[i]) {
            numPiecesPos++;
  }
        }
        boolean moreThanOnePieceChanged = (numPiecesScanned > numPiecesPos + 1) || (numPiecesPos > numPiecesScanned + 1);


        //check for minimum repetitions to copy to scanned
        //On Lichess as it is self-correcting it is not that important
        if (/*(isLichessConnected && numScansV2) ||*/ (numScansV2 >= MIN_SCANS_V2)) {

          //If more than one piece changed, we scan more times. When it is only one it is not that important
          //because if it is a glitch it will auto-correct when the piece is back
          if (!moreThanOnePieceChanged || (numScansV2 >= MIN_SCANS_V2_MULTIPLE))  {
            for (int i = 0; i < 64; i++) {
              scannedpos[i] = scannedposV2[i];
            }
            numScansV2 = 0;
          }
        }
      }




  //Display leds
  displayLedRow();

      //RELEASE SPI BUS
      if (isSemaphoreTaken) {
        xSemaphoreGive( ( xScreenSemaphore ) );
      }




  //Start of board movement detection comparing new scanned board positions with previous ones

  //Check if inverting board lifting both kings in the initial position
      if (!chess960ModeEnabled && !firstScan && ! boardInverted && boardInInitialPos && !scannedpos[59] && !scannedpos[3]
          && scannedpos[0]
          && scannedpos[1]
          && scannedpos[2]
          && scannedpos[4]
          && scannedpos[5]
          && scannedpos[6]
          && scannedpos[7]
          && scannedpos[8]
          && scannedpos[9]
          && scannedpos[10]
          && scannedpos[11]
          && scannedpos[12]
          && scannedpos[13]
          && scannedpos[14]
          && scannedpos[15]
          && scannedpos[63]
          && scannedpos[62]
          && scannedpos[61]
          && scannedpos[60]
          && scannedpos[58]
          && scannedpos[57]
          && scannedpos[56]
          && scannedpos[55]
          && scannedpos[54]
          && scannedpos[53]
          && scannedpos[52]
          && scannedpos[51]
          && scannedpos[50]
          && scannedpos[49]
          && scannedpos[48]

         ) {


        if (serialLog) Serial.println("REVERSING BOARD KINGS LIFTED");

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

        if (!chess960ModeEnabled && !firstScan && boardInverted && boardInInitialPos && !scannedpos[59] && !scannedpos[3]
            && scannedpos[0]
            && scannedpos[1]
            && scannedpos[2]
            && scannedpos[4]
            && scannedpos[5]
            && scannedpos[6]
            && scannedpos[7]
            && scannedpos[8]
            && scannedpos[9]
            && scannedpos[10]
            && scannedpos[11]
            && scannedpos[12]
            && scannedpos[13]
            && scannedpos[14]
            && scannedpos[15]
            && scannedpos[63]
            && scannedpos[62]
            && scannedpos[61]
            && scannedpos[60]
            && scannedpos[58]
            && scannedpos[57]
            && scannedpos[56]
            && scannedpos[55]
            && scannedpos[54]
            && scannedpos[53]
            && scannedpos[52]
            && scannedpos[51]
            && scannedpos[50]
            && scannedpos[49]
            && scannedpos[48]   ) {

          if (serialLog) Serial.println("UNREVERSING BOARD KINGS LIFTED");

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
            !chess960ModeEnabled &&
        boardInInitialPos && (i == 59 || i == 3 || i == 60 || i == 4 )) {
        //Lifting king in intial pos, maybe want to invert board
        break;
            //      } else if (boardInverted && boardInInitialPos && (i == 60 || i == 4)) {
            //        //Lifting king in intial pos, maybe want to invert board
            //        break;
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

      if (numLifted || numPositioned) {
        for (int i = 0; i < 8; i++) {
          for (int j = 0; j < 8; j++) {
            if (serialLog) Serial.print(scannedpos[(8 * i) + j ],  DEC);
          }
          if (serialLog) Serial.println("");
        }

        if (serialLog) Serial.print("ROWS SCANNED: ");
        if (serialLog) Serial.println(numRowsScanned, DEC);
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
          //TBM
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
          //TBM
      boardIsOutOfSync = true;
    } else if (liftedPiece != 0 || liftedPiece2 != 0) {
      //lost track
      //signal out-of sync to the user and ask to compose the position scanning the missing pieces
          //TBM
      boardIsOutOfSync = true;
    }
    liftedPiece = previouslyLifted;
    liftedPieceSquare = previouslyLiftedSquare;
    liftedPiece2 = lastLifted;
    liftedPiece2Square = lastLiftedSquare;
  }



      //In chess960 we check if there are only pawns in the starting rows. In that case we assume the lifted piece is in the sequence
      //first white, then black
      //King-Queen-2 Rooks-2 Bishops-2 Horses

      if (chess960ModeEnabled) {

        //check if there are pieces only in the starting rows, and all pawns are present
        boolean allPiecesInStartingRows = true;
        int countWhitePieces = 0;
        int countBlackPieces = 0;
        for (int i = 16; i <= 47; i++) {
          if (scannedpos[i]) {
            allPiecesInStartingRows = false;
          }
        }
        for (int i = 8; i <= 15; i++) {
          if (!scannedpos[i]) {
            allPiecesInStartingRows = false;
          }
        }

        for (int i = 48; i <= 55; i++) {
          if (!scannedpos[i]) {
            allPiecesInStartingRows = false;
          }
        }

        for (int i = 56; i <= 63; i++) {
          if (scannedpos[i]) {
            countWhitePieces++;
          }
        }

        for (int i = 0; i <= 7; i++) {
          if (scannedpos[i]) {
            countBlackPieces++;
          }
        }


        if (countWhitePieces == 0 && countBlackPieces == 0) {
          chess960ModeAlreadySetUp = false; 
        }

        

        //Set all pawns
        if (allPiecesInStartingRows && !chess960ModeAlreadySetUp) {

          for (int i = 8; i <= 15; i++) {
            currentPos[i] = initpos[i];
          }
          for (int i = 48; i <= 55; i++) {
            currentPos[i] = initpos[i];
          }

          for (int i = 56; i <= 63; i++) {
            if (scannedpos[i] && !currentPos[i]) {

              numPositioned = 0;

              if (serialLog) Serial.print("White pieces: ");
              if (serialLog) Serial.println(countWhitePieces, DEC);

              switch (countWhitePieces) {
                case 1: currentPos[i] = WKING;
                  break;
                case 2: currentPos[i] = WQUEEN;
                  break;
                case 3: currentPos[i] = WROOK;
                  break;
                case 4: currentPos[i] = WROOK;
                  break;
                case 5: currentPos[i] = WBISHOP;
                  break;
                case 6: currentPos[i] = WBISHOP;
                  break;
                case 7: currentPos[i] = WKNIGHT;
                  break;
                case 8: currentPos[i] = WKNIGHT;
                  break;
                default:
                  break;
              }
            }
          }


          for (int i = 0; i <= 7; i++) {
            if (scannedpos[i] && !currentPos[i]) {

              numPositioned = 0;

              if (serialLog) Serial.print("Black pieces: ");
              if (serialLog) Serial.println(countBlackPieces, DEC);

              switch (countBlackPieces) {
                case 1: currentPos[i] = BKING;
                  break;
                case 2: currentPos[i] = BQUEEN;
                  break;
                case 3: currentPos[i] = BROOK;
                  break;
                case 4: currentPos[i] = BROOK;
                  break;
                case 5: currentPos[i] = BBISHOP;
                  break;
                case 6: currentPos[i] = BBISHOP;
                  break;
                case 7: currentPos[i] = BKNIGHT;
                  break;
                case 8: currentPos[i] = BKNIGHT;
                  break;
                default:
                  break;
              }
            }
          }
        }


        if (countWhitePieces == 8 && countBlackPieces == 8) {
          chess960ModeAlreadySetUp = true; 
        }
      }




  //If more than one piece is positioned, or one piece is positioned when no piece was raised we have lost track of the game
  //Unless we are sertting up the initial position, in that case we know what the pieces are
      if ((firstScan && !startLichessConnection) || settingUpBoardMode ||  (numPositioned > 0 && liftedPiece == 0)) {


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

          if (!firstScan && !settingUpBoardMode && !chess960ModeEnabled) {
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
          //TBM
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
                //signal out of sync board to urge user to position the missing piece
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
            if (!chess960ModeEnabled) {
        for (int j = 0; j <= 63; j++) {
          if (currentPos[j] != initpos[j]) {
            sendUpdate = true;
          }
          currentPos[j] = initpos[j];
        }
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


        unsigned long now = millis();

        if (now > timeScanRfid) {
    if (programRfidTag(missingPiece)) {
      //Successfully programmed
      boop();
      beep();
      boop();
    }
          timeScanRfid = millis() + rfid_scan_interval;
        }
  }






  //BLE status update
  long currentTime = millis();

      if (BLECdeviceConnected && currentTime > (lastUpdateTimeTypeBUSBBoard +  200 )) {
        //Update every 200ms
        sendStatusIfNeeded();
        lastUpdateTimeTypeBUSBBoard = millis();
      } else if (!disableStatusOnEveryScan && updateModeTypeBUSBBoard == 0 && currentTime > (lastUpdateTimeTypeBUSBBoard +  1000 )) {
    //Update every scan
    sendStatusIfNeeded();
        lastUpdateTimeTypeBUSBBoard = millis();

  } else if (updateModeTypeBUSBBoard == 2 && currentTime > (lastUpdateTimeTypeBUSBBoard +  updateIntervalTypeBUSBBoard ))  {
    //Timed update, update only if enough time has passed since last update
    sendStatusIfNeeded();
        lastUpdateTimeTypeBUSBBoard = millis();

      } else if ( updateModeTypeBUSBBoard != 1 && (numChanges > 0 || sendUpdate) ) {
        //Update on change always
    sendStatusIfNeeded();
        lastUpdateTimeTypeBUSBBoard = millis();
  }


  //if (serialLog) Serial.println("LI0");

  //Data back to Mode A driver
  if (!firstCharacterReceived && !isTypeBUSBBoard) {
    sendPendingUpdates();
  } else {
    numChanges = 0;
  }


      //Search for NFC tags to identify lifted pieces
      //Ignore if we are in programming mode
      if ( !(missingPiece && missingPiece < 99)) {

        unsigned long now = millis();
        if (now > timeScanRfid) {

          if (isLichessConnected) {
            if (readRfidTag(&lastScannedPieceLichess)) {
              boop();
            }
          } else {
            if (readRfidTag(&liftedPiece)) {
              boop();
            }
          }
          timeScanRfid = millis() + rfid_scan_interval;
        }
      }

    }//END OF SPI CRITICAL SECTION

  }//END OF SCAN CODE if (now > timeScan)

  //Lichess mode

  if (startLichessConnection) {
    if (serialLog) Serial.println("Searching for running games");
    while (!getRunningGameLichess()) {
      if (serialLog) Serial.println("Retrying getRunningGameLichess");
    }
    isLichessConnected = true;
    startLichessConnection = false;
  }

  if (isLichessConnected) {

    if (isLichessGameActive()) {
      //getIncomingGameEventsLichess();
      fixBoardPositionLichessData();
      if (isLichessGameRunning()) {
      setLichessBoardLeds();
      refreshLichessClocks();
      }
    } else {
      //getLichessTvEvents();
      clearAllBoardLeds();
      refreshLichessClocks();
    }

    unsigned long now = millis();
    if (now > timeUpdateSprites) {
    refreshLichessPlayerTopSprite();
    refreshLichessPlayerBottomSprite();
      timeUpdateSprites = millis() +  sprite_update_interval;
    }
  }


  //Refresh display
  if (!isLichessConnected || isLichessFullyInitialized() ) {
  displayBoard();
  }

  //Read module buttons
  now = millis();
  if (now > timeUpdateButtons) {
  read_module_buttons();
    timeUpdateButtons = millis() +  buttons_update_interval;
  }
  

  //We block here if there are pending external clock messages to the host
  if (isExternalClock) {
    checkForPendingExternalClockMessages();
  }

  //END OF MAIN LOOP

}
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
