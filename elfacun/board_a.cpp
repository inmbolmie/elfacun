/*
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license

    This program is distributed WITHOUT ANY WARRANTY, even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "board_a.h"
#include "serial.h"
#include "clock.h"


//Magic board data for driver

//Trademark
char* tmString = "Elfacun https://github.com/inmbolmie/elfacun";

//Version
byte vermajor = 0;
byte verminor = 60;

//Clock version
byte verclockmajor = 2;
byte verclockminor = 0;

//Serial short and long version
char* shortSerialNumber = "00001";
char* serialNumber = "3.24000000";

//Current board update mode
byte updateMode = 0;

//Buttons

boolean pushedBack = false;
boolean pushedMinus = false;
boolean pushedPlay = false;
boolean pushedPlus = false;
boolean pushedNext = false;
byte lastbuttons = 0b11111111;
byte pushButtonCount = 0;
boolean adjustingBrightness = false;
boolean disableSendButtons = false;

boolean holdMode = false;
boolean slowMode = false;
boolean disableBLEmode = false;





//LED TIMER
//It fires every 50ms to update led pattern on board

hw_timer_t * timerLed = NULL;
portMUX_TYPE timerLedMux = portMUX_INITIALIZER_UNLOCKED;


void IRAM_ATTR onTimerLed() {
  // Increment the counter and set the time of ISR
  portENTER_CRITICAL_ISR(&timerLedMux);

  millisLed += 50;

  if (millisLed > ledInterval) {
    millisLed = 0;
    currentLedPage++;
    if (currentLedPage >= 8) {
      currentLedPage = 0;
    }
  }

  for (int i = 0; i < 64; i++) {

    if (!boardInverted) {
      ledStatus[i] = 0;
    } else {
      ledStatus[63 - i] = 0;
    }

    for (int j = 0; j < 8; j++) {
      //If lit in any page it will be lit and rows cycled, due to hardware limitations
      if (ledStatusBufferX[i] > 0 || (ledStatusBuffer[(64 * j) + i] == 4) || (ledStatusBufferB[(64 * j) + i] == 4) ) {
        if (!boardInverted) {
          ledStatus[i] = 4;
        } else {
          ledStatus[63 - i] = 4;
        }
      }
    }
  }

  portEXIT_CRITICAL_ISR(&timerLedMux);
}






//Send clock data to the driver
void send_clock() {

  if (serialLog) Serial.println("SENDING CLOCK DATA TO SERIAL");
  writeSerial(0x8D);
  writeSerial(0x00);
  writeSerial(0x0A);

  byte hours = rhours | (rFlagSymbol << 4); //FIXME
  byte minutes = decToBcd(rmins);
  byte seconds = decToBcd(rsecs);
  writeSerial(hours);
  writeSerial(minutes);
  writeSerial(seconds);

  //if (serialLog) Serial.print(hours, HEX);
  //if (serialLog) Serial.print(" ");
  //if (serialLog) Serial.print(minutes, HEX);
  //if (serialLog) Serial.print(" ");
  //if (serialLog) Serial.print(seconds, HEX);
  //if (serialLog) Serial.print(" ");

  hours = lhours | (lFlagSymbol << 4); //FIXME
  minutes = decToBcd(lmins);
  seconds = decToBcd(lsecs);
  writeSerial(hours);
  writeSerial(minutes);
  writeSerial(seconds);


  //if (serialLog) Serial.print(lhours, HEX);
  //if (serialLog) Serial.print(" ");
  //if (serialLog) Serial.print(minutes, HEX);
  //if (serialLog) Serial.print(" ");
  //if (serialLog) Serial.print(seconds, HEX);
  //if (serialLog) Serial.print(" ");

  //if (serialLog) Serial.println("");

  byte clockState = (rRunning || rRunning) | (leverPositionHighRight << 1) | (batSymbol << 2) | (rightTurn << 3) | (!rightTurn << 3);
  writeSerial(clockState);

  commitSerial();
}





//Responses
void send_clock_ack(boolean response, boolean error, byte type, byte parameter) {
  writeSerial(0x8D);
  writeSerial(0x00);
  writeSerial(0x0A);

  byte ack0 = 0x10;
  if (error) {
    ack0 = ack0 << 2;
  }
  byte ack1 = ((!response) << 7) | type;
  byte ack2 = 0x00;
  byte ack3 = 0x00;
  switch (type) {
    case 9:
      //version
      ack2 = (verclockmajor << 4) | (verclockminor & 0x0F);
      break;
    case 8:
      //button response
      ack3 = 0x00;
      break;
    case 0x88:
      //button push
      ack3 = parameter;
      break;
    default:
      break;
  }

  //encode ack
  //byte3
  writeSerial((ack2 & 0x80) >> 3 |  (ack3 & 0x80) >> 2  | 0x0A);
  //byte4
  writeSerial(ack0 & 0x7f);
  //byte5
  writeSerial(ack1 & 0x7f);
  //byte6
  writeSerial((ack0 & 0x80) >> 3  |  (ack1 & 0x80) >> 2);
  //byte7
  writeSerial(ack2 & 0x7f);
  //byte8
  writeSerial(ack3 & 0x7f);
  //byte9
  writeSerial(0x00);

  commitSerial();
}

void send_clock_version() {
  send_clock_ack(true, false, 9, NULL);
}


void send_display_ack() {
  send_clock_ack(true, false, 1, NULL);
}


void send_button_state_response(byte button) {
  send_clock_ack(true, false, 0x08, 0x00);
}


void send_button_push(byte button) {

  //Button pushes for lichess
  if (isLichessConnected) {
    lichessButtonPush(button);
    return;
  }

  if (button == BUTTON_LEVER) {
    leverPositionHighRight = !leverPositionHighRight;
    if (serialLog) Serial.print ("SEND LEVER CHANGE: ");
    if (serialLog) Serial.println (leverPositionHighRight, DEC);
    send_clock();

  } else {
    if (serialLog) Serial.print ("SEND PHYS BUTTON: ");
    if (serialLog) Serial.println (button, DEC);
    send_clock_ack(false, false, 0x88, button);
  }
}


void send_control_ack() {
  send_clock_ack(true, false, 10, NULL);
}


void set_clock(byte * content) {

  //int lmillis = 0;
  //byte lhours = 0;
  //byte lmins = 0;
  //byte lsecs = 0;
  //byte lcents = 0;
  //boolean lCountsDown=true;

  //boolean lTimeSymbol = false;
  //boolean lFischSymbol = false;
  //boolean lDelaySymbol = false;
  //boolean lHglassSymbol = false;
  //boolean lUpcntSymbol = false;
  //boolean lByoSymbol = false;
  //boolean lEndSymbol = false;
  //boolean lPeriod = 0;
  //boolean lFlagSymbol = false;
  //boolean lSoundSymbol = false;
  //boolean lBlackSymbol = false;
  //boolean keepIcons=false;
  //boolean batSymbol = false;

  lhours = content[0];
  lmins = content[1];
  lsecs = content[2];
  rhours = content[3];
  rmins = content[4];
  rsecs = content[5];

  lmillis = (lhours * 3600000) + (lmins * 60000) + (lsecs * 1000);
  rmillis = (rhours * 3600000) + (rmins * 60000) + (rsecs * 1000);

  byte status = content[6];
  lCountsDown = status & 0x01;
  rCountsDown = (status & 0x02) >> 1;
  if (status & 0x04) {
    lRunning = false;
    rRunning = false;
  } else {
    if (lCountsDown) {
      lRunning = true;
    }

    if (rCountsDown) {
      rRunning = true;
    }

  }

  if (!displayingMessage && !isExternalClock) {
    display_clock();
  }


  if (isExternalClock) {
    externalClockSetAndRun(lhours, lmins, lsecs, lCountsDown, lRunning, rhours, rmins, rsecs, rCountsDown, rRunning);
  }

}




void sendPong() {
  writeSerial(0x80);
  commitSerial();
}


void sendSquareChange(int square, int newPiece) {
  writeSerial(0x8E);
  writeSerial(0x00);
  writeSerial(0x05);
  writeSerial(square);
  writeSerial(newPiece);
  commitSerial();
}


void sendPendingUpdates() {
  while (numChanges > 0) {
    sendSquareChange(changes[numChanges - 1].square, changes[numChanges - 1].newPiece);

    if (serialLog) Serial.print("ENVIO MOVIMIENTO PIEZA: ");
    if (serialLog) Serial.print(changes[numChanges - 1].square, DEC);
    if (serialLog) Serial.print(" ");
    if (serialLog) Serial.println(changes[numChanges - 1].newPiece, DEC);
    numChanges--;
  }
}


void sendBoardDump() {


  writeSerial(0x86);
  writeSerial(0x00);
  writeSerial(0x43);
  for (int i = 0; i < 64; i++) {
    writeSerial(currentPos[i]);
  }
  commitSerial();

  if (serialLog) Serial.println("ENVIO TABLERO COMPLETO");
}


void sendShortSerial() {
  writeSerial(0x91);
  writeSerial(0x00);
  writeSerial(0x08);

  printSerial(shortSerialNumber, 5);

  commitSerial();

}


void sendSomeAddress() {
  writeSerial(0x90);
  writeSerial(0x00);
  writeSerial(0x05);
  writeSerial(0x00);
  writeSerial(0x00);

  commitSerial();
}


void sendADescription() {
  writeSerial(0x92);
  writeSerial(((3 + strlen(tmString)) & 0x80) >> 7);
  writeSerial((3 + strlen(tmString)) & 0x7F);
  printSerial(tmString);

  commitSerial();
}


void sendBoardVersion()
{
  writeSerial(0x93);
  writeSerial(0x00);
  writeSerial(0x05);
  writeSerial(vermajor);
  writeSerial(verminor);

  commitSerial();
}


void sendImok() {
  writeSerial(0xA0);
  writeSerial(0x00);
  writeSerial(0x0C);
  writeSerial(0x64);
  writeSerial(0x7F);
  writeSerial(0x7F);
  writeSerial(0x00);
  writeSerial(0x00);
  writeSerial(0x00);
  writeSerial(0x00);
  writeSerial(0x00);
  writeSerial(0x00);

  commitSerial();
}

void  sendLongSerial() {

  writeSerial(0xA2);
  writeSerial(0x00);
  writeSerial(0x0D);

  printSerial(serialNumber, 10);

  commitSerial();

}



void board_a_clock_loop(byte command, byte * content, byte size ) {

  //Process clock messages
  switch (command) {
    case 1:
      //Set display clock version 1
      send_display_ack();
      break;
    case 2:
      //Set icons
      send_display_ack();
      break;
    case 3:
      //clear
      displayingMessage = false;
      if (isExternalClock) {
        externalClockEndDisplay();
      } else {
        clear_message();
        display_clock();
      }
      send_display_ack();
      break;
    case 8:
      //push
      send_button_state_response(NULL);
      break;
    case 9:
      //version
      send_clock_version();
      break;
    case 10:
      //control
      set_clock(content);
      send_control_ack();
      break;
    case 11:
      beep();
      //sound
      break;
    case 12:
    case 13:
      //message to clock version 2
      if (serialLog) Serial.print ("DISPLAY TEXT: ");
      displayingMessage = true;
      content[size - 4] = 0;
      String myString = String((char *)content);
      if (serialLog) Serial.println(myString);

      if (isExternalClock) {
        externalClockSetDisplay(myString);
      } else {
        display_message(myString);
      }

      send_display_ack();
      break;

  }

}





void send_board() {
  sendBoardDump();
}


void send_short_serial() {
  sendShortSerial();
}

void send_somesaddress() {
  sendSomeAddress();

}

void send_a_description() {
  sendADescription();

}

void send_moves() {
  //tft.println("SEND_MOVES");

}

void set_update() {
  updateMode = UPDATE_NORMAL;
}

void set_update_board() {
  updateMode = UPDATE_BOARD;

}

void set_update_lazy() {
  updateMode = UPDATE_LAZY;

}

void send_imok() {
  sendImok();

}

void send_board_version() {
  sendBoardVersion();

}

void send_black() {
  //NOT IMPLEMENTED
}

void scan_black() {
  //NOT IMPLEMENTED
}

void send_white() {
  //NOT IMPLEMENTED
}

void scan_white() {
  //NOT IMPLEMENTED
}

void scan_all() {
  //NOT IMPLEMENTED
}

void send_long_serial() {
  sendLongSerial();
}




void process_clock_message () {
  //Read rest of message
  if (serialLog) Serial.print ("BOARD RECEIVED CLOCK MESSAGE: ");
  byte size = readSerialBlock();

  readSerialBlock();
  byte command = readSerialBlock();
  if (serialLog) Serial.print (command, HEX);
  if (serialLog) Serial.print (" SIZE: ");
  if (serialLog) Serial.print (size, DEC);
  if (serialLog) Serial.print (" ");
  byte content[size - 5];
  for (int i = 0; i <= size - 4; i++) {
    //Content
    content[i] = readSerialBlock();
    if (serialLog) Serial.print (content[i], HEX);
    if (serialLog) Serial.print ("_");
  }
  if (serialLog) Serial.println("");
  readSerialBlock();

  board_a_clock_loop(command, content, size);
}





void set_leds() {

  //Read rest of message
  if (serialLog) Serial.print ("BOARD RECEIVED LED MESSAGE: ");
  byte size = readSerialBlock();
  byte pattern;
  byte startField;
  byte endField;

  if (size == 0x02) {

    pattern = readSerialBlock();
    readSerialBlock();

    if (serialLog) Serial.print (size, DEC);
    if (serialLog) Serial.print (" ");
    if (serialLog) Serial.print (pattern, DEC);
    if (serialLog) Serial.println ("");

  } else {

    pattern = readSerialBlock();
    startField = readSerialBlock();
    endField = readSerialBlock();
    readSerialBlock();

    if (serialLog) Serial.print (size, DEC);
    if (serialLog) Serial.print (" ");
    if (serialLog) Serial.print (pattern, DEC);
    if (serialLog) Serial.print (" ");
    if (serialLog) Serial.print (startField, DEC);
    if (serialLog) Serial.print (" ");
    if (serialLog) Serial.print (endField, DEC);
    if (serialLog) Serial.println ("");

  }

  //size == 0x02 trick for compatibility with old versions of BearChess
  if (size == 0x02 || startField == 0x40) {
    startField = 0;
    endField = 63;

    for (int i = startField; i <= endField; i++) {
      for (int j = 0; j < 8; j++) {
        ledStatusBuffer[i + (64 * j)] = pattern * 4;
      }
    }
  } else if (pattern == 2) {
    if (startField == 0x01 and endField == 0x00) {
      modeAautoLedsOff = false;
    }

    else if (startField == 0x01 and endField == 0x01) {
      modeAautoLedsOff = true;
    }

    else if (startField == 0x02 and endField == 0x01) {
      modeAautoReverseLeds = false;
    }

    else if (startField == 0x02 and endField == 0x01) {
      modeAautoReverseLeds = true;
    }
  }
  else {
    for (int j = 0; j < 8; j++) {
      ledStatusBuffer[(63 - startField) + (64 * j)] = pattern * 4;
      ledStatusBuffer[(63 - endField) + (64 * j)] = pattern * 4;
    }
  }
}



void board_a_command_loop(byte rcv) {
  switch (rcv) {

    //No need to answer
    case MODE_A_RESET:
      reset();
      break;

    //I expect a reply
    case MODE_A_CLOCK_REQUEST:
      send_clock();
      break;

    case MODE_A_CLOCK_COMMAND:
      process_clock_message();
      break;

    case MODE_A_BOARD_REQUEST:
      send_board();
      break;

    case MODE_A_UPDATE_REQUEST:
      set_update();
      break;

    case MODE_A_UPDATE_BOARD_REQUEST:
      set_update_board();
      break;

    case MODE_A_SERIAL_REQUEST:
      send_short_serial();
      break;

    case MODE_A_ADDRESS_REQUEST:
      send_somesaddress();
      break;

    case MODE_A_DESCRIPTION_REQUEST:
      send_a_description();
      break;

    case MODE_A_MOVES_REQUEST:
      send_moves();
      break;

    case MODE_A_UPDATE_LAZY:
      set_update_lazy();
      break;

    case MODE_A_BATTERY_LEVEL:
      send_imok();
      break;

    case MODE_A_VERSION_REQUEST:
      send_board_version();
      break;

    case MODE_A_BOARD_REQUEST_BLACK:
      send_black();
      break;

    case MODE_A_BOARD_SCAN_BLACK:
      scan_black();
      break;

    case MODE_A_BOARD_REQUEST_WHITE:
      send_white();
      break;

    case MODE_A_BOARD_SCAN_WHITE:
      scan_white();
      break;

    case MODE_A_BOARD_SCAN_ALL:
      scan_all();
      break;

    case MODE_A_LONG_SERIAL_REQUEST:
      send_long_serial();
      break;

    case MODE_A_LEDS:
      isModeALeds = true;
      set_leds();
      break;

    default:
      if (serialLog) Serial.print("---------------->ERROR UNKNOWN COMMAND RECEIVED: ");
      if (serialLog) Serial.println(rcv, HEX);
      break;
  }

  if (serialLog) Serial.print("Received command: ");
  if (serialLog) Serial.println(rcv, HEX);

}


void read_module_buttons() {

  //GET SPI BUS
  boolean isSemaphoreTaken = true;
  if (!xSemaphoreTake( xScreenSemaphore, ( TickType_t ) screenSemaphoreTimeout )) {
    isSemaphoreTaken = false;
    if (serialLog) Serial.println("XXXXXXXXXXXXXXX   ERROR TAKING SEMAPHORE AT read_module_buttons, skipping loop ");

  } else {

    byte  buttons = SPIExpanderButtons.readPort(0);

    boolean stuck = false;
    if ((buttons & 0b00111111) != 0b00111111) {
      if (serialLog) Serial.print("Pushing buttons: ");
      if (serialLog) Serial.println((buttons & 0b00111111), BIN);


      //Try to detect stuck expanders
      int count=0;
      for (uint8_t i = 1; i > 0; i <<= 1) {
        if (buttons & i) ++count;
      }

      if (count <=3 ) {
        stuck = true;
      }
    }

    
    if (stuck) {

      if (serialLog) Serial.print("Bus expander selected: ");
      if (serialLog) Serial.println(digitalRead(CONTROLLER_OUTPUT_CHIPSELECT) , DEC);

      if (serialLog) Serial.print("Buttons expander selected: ");
      if (serialLog) Serial.println(digitalRead(CONTROLLER_OUTPUT_CHIPSELECT_BUTTONS) , DEC);

      
      
      if (serialLog) Serial.println("RESETTING SPI EXPANDERS------------------------------------------");
      SPIExpanderButtons.begin();
      delay(100);
      SPIExpander.begin();
      delay(100);
      buttons = SPIExpanderButtons.readPort(0);

      if (serialLog) {
        buup();
        delay(50);
        buup();
        delay(50);
        buup();
        delay(50);
        buup();
        delay(50);
        buup();
        delay(50);
        buup();
        delay(50);
        buup();
      }
      
      if (serialLog) Serial.print("Pushing buttons: ");
      if (serialLog) Serial.println((buttons & 0b00111111), BIN);

    }

    boolean sendButton = true;

    //Check for brightness adjustment
    if ((pushButtonCount >= 20 || (adjustingBrightness && pushButtonCount >= 2)) && ! (buttons & 0b00000001)) {
      pushButtonCount = 0;
      decreaseTftBrightness();
      adjustingBrightness = true;
    }
    if ((pushButtonCount >= 20 || (adjustingBrightness && pushButtonCount >= 2)) && ! (buttons & 0b00100000)) {
      pushButtonCount = 0;
      increaseTftBrightness();
      adjustingBrightness = true;
    }

    //Check for hold position mode deactivation
    if (holdMode && ((!slowMode && pushButtonCount < 20) || (slowMode && pushButtonCount < 3)) && ((buttons & 0b00111111) != 0b00111111)) {
      holdMode = false;
      pushButtonCount = 0;
      cancelLichessConfirmation();
      deleteNVSBoardPosition();
      disableSendButtons = false;
      sendButton = false;

      while ((buttons & 0b00111111) != 0b00111111) {
        buttons = SPIExpanderButtons.readPort(0);
      }
    }

    //Check for hold position mode activation
    if (!holdMode && ((!slowMode && pushButtonCount >= 20) || (slowMode && pushButtonCount >= 3)) && !isLichessConnected && !(buttons & 0b00000010)) {
      if (!holdMode) {
        announceHoldMode();
        storeNVSBoardPosition();
      }
      holdMode = true;
      disableSendButtons = true;
    }


    //Check for 960 mode activation
    if (!chess960ModeEnabled && !disableSendButtons && ((!slowMode && pushButtonCount >= 20) || (slowMode && pushButtonCount >= 3)) && !isLichessConnected && !(buttons & 0b00000100)) {
      chess960ModeEnabled = true;
      onChess960ModeIcon();
      disableSendButtons = true;
      boardInverted = false;
    }


    //Check for 960 black in front mode activation
    if (chess960ModeEnabled && disableSendButtons && ((!slowMode && pushButtonCount >= 60) || (slowMode && pushButtonCount >= 9)) && !isLichessConnected && !(buttons & 0b00000100)) {
      boardInverted = true;
    }


    //Check for 960 mode deactivation
    if (chess960ModeEnabled && !disableSendButtons && ((!slowMode && pushButtonCount >= 20) || (slowMode && pushButtonCount >= 3)) && !isLichessConnected && !(buttons & 0b00000100)) {
      chess960ModeEnabled = false;
      offChess960ModeIcon();
      disableSendButtons = true;
      boardInverted = false;
    }
    

    //Check for disable/enable BLE mode activation
    if ( ((!slowMode && pushButtonCount >= 20) || (slowMode && pushButtonCount >= 3)) && !isLichessConnected && !(buttons & 0b00010000)) {


      if ((!slowMode && pushButtonCount == 20) || (slowMode && pushButtonCount == 3)) {
        if (!BLEdeviceConnected) {
          if (!disableBLEmode) {
            onBLEConnectedIconDisabled();
            disableBLEmode = true;
            stopAdvertisement();
          } else {
            startAdvertisement();
            offBLEConnectedIcon();
            disableBLEmode = false;
          }
        }
      }

      disableSendButtons = true;

    }


    if (sendButton && ! adjustingBrightness && ! disableSendButtons && !isLichessConnected && (strcmp(revelationButtonOrder, "TRUE") == 0)) {
      //First button is change lever
      if (buttons & 0b00000001 && ! (lastbuttons & 0b00000001)) {
        send_button_push(BUTTON_LEVER);
      }
      if (buttons & 0b00000010 && ! (lastbuttons & 0b00000010)) {
        send_button_push(BUTTON_BACK);
      }
      if (buttons & 0b00000100 && ! (lastbuttons & 0b00000100)) {
        send_button_push(BUTTON_MINUS);
      }
      if (buttons & 0b00001000 && ! (lastbuttons & 0b00001000)) {
        send_button_push(BUTTON_PLAY);
      }
      if (buttons & 0b00010000 && ! (lastbuttons & 0b00010000)) {
        send_button_push(BUTTON_PLUS);
      }
      if (buttons & 0b00100000 && ! (lastbuttons & 0b00100000)) {
        send_button_push(BUTTON_NEXT);
      }
    } else if (sendButton && ! adjustingBrightness && ! disableSendButtons) {
      if (buttons & 0b00000001 && ! (lastbuttons & 0b00000001)) {
        send_button_push(BUTTON_BACK);
      }
      if (buttons & 0b00000010 && ! (lastbuttons & 0b00000010)) {
        send_button_push(BUTTON_MINUS);
      }
      if (buttons & 0b00000100 && ! (lastbuttons & 0b00000100)) {
        send_button_push(BUTTON_PLAY);
      }
      if (buttons & 0b00001000 && ! (lastbuttons & 0b00001000)) {
        send_button_push(BUTTON_PLUS);
      }
      if (buttons & 0b00010000 && ! (lastbuttons & 0b00010000)) {
        send_button_push(BUTTON_NEXT);
      }
      if (buttons & 0b00100000 && ! (lastbuttons & 0b00100000)) {
        send_button_push(BUTTON_LEVER);
      }
    }

    if (! (buttons & 0b00000001) && ! (lastbuttons & 0b00000001)) {
      pushButtonCount++;
    } else if (! (buttons & 0b00100000) && ! (lastbuttons & 0b00100000)) {
      pushButtonCount++;
    } else if (! (buttons & 0b00000010) && ! (lastbuttons & 0b00000010)) {
      pushButtonCount++;
    } else if (! (buttons & 0b00000100) && ! (lastbuttons & 0b00000100)) {
      pushButtonCount++;
    } else if (! (buttons & 0b00010000) && ! (lastbuttons & 0b00010000)) {
      pushButtonCount++;
    } else {
      pushButtonCount = 0;
      if (adjustingBrightness) {
        storeAdjustedBrightness();
      }
      adjustingBrightness = false;

      disableSendButtons = false;

    }


    lastbuttons = buttons;


    //RELEASE SPI BUS
    xSemaphoreGive( ( xScreenSemaphore ) );

  }

}
