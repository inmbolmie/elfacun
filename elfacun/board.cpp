/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "board.h"



Movement *lastMovement = NULL;

boolean boardIsOutOfSync = false;

SquareChange changes[100];
int numChanges = 0;

byte initpos[64] = {8, 9, 10, 12, 11, 10, 9, 8,
                    7, 7, 7, 7, 7, 7, 7, 7,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    0, 0, 0, 0, 0, 0, 0, 0,
                    1, 1, 1, 1, 1, 1, 1, 1,
                    2, 3, 4, 6, 5, 4, 3, 2
                   };

byte lastDrawn[64] = {255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255,
                      255, 255, 255, 255, 255, 255, 255, 255
                     };


byte lastDrawnBG[64] = {0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1,
                        0, 1, 0, 1, 0, 1, 0, 1
                       };

boolean lastpos[64] = { true, true, true, true, true, true, true, true,
                        true, true, true, true, true, true, true, true,
                        false, false, false, false, false, false, false, false,
                        false, false, false, false, false, false, false, false,
                        false, false, false, false, false, false, false, false,
                        false, false, false, false, false, false, false, false,
                        true, true, true, true, true, true, true, true,
                        true, true, true, true, true, true, true, true,
                      };

boolean scannedpos[64] = { true, true, true, true, true, true, true, true,
                           true, true, true, true, true, true, true, true,
                           false, false, false, false, false, false, false, false,
                           false, false, false, false, false, false, false, false,
                           false, false, false, false, false, false, false, false,
                           false, false, false, false, false, false, false, false,
                           true, true, true, true, true, true, true, true,
                           true, true, true, true, true, true, true, true,
                         };


boolean transposescannedpos[64];


boolean passivescannedpos[64] = { true, true, true, true, true, true, true, true,
                                  true, true, true, true, true, true, true, true,
                                  false, false, false, false, false, false, false, false,
                                  false, false, false, false, false, false, false, false,
                                  false, false, false, false, false, false, false, false,
                                  false, false, false, false, false, false, false, false,
                                  true, true, true, true, true, true, true, true,
                                  true, true, true, true, true, true, true, true,
                                };


byte ledStatusBuffer[8 * 64];
byte ledStatus[64];
byte ledInterval = 4000;
boolean isModeALeds = false;
boolean modeAautoLedsOff = true;
boolean modeAautoReverseLeds = true;
boolean ledUpdateReceived = false;
int millisLed = 0;
int ledIntervalCycles = 0;
boolean sendFullUpdateOnNextInitialPos = false;
boolean ledsAreDisabled = false;


byte liftedPiece = 0;
byte liftedPieceSquare = 0;
byte liftedPiece2 = 0;
byte liftedPiece2Square = 0;
byte promotingSquare = 100;
boolean boardInInitialPos = true;
boolean settingUpBoardMode = false;

boolean takingBackMovement = false;

byte currentPos[64] ;

boolean boardInverted = false;


//BLE objects

boolean BLEdeviceConnected = false;
std::vector<byte> ledbuffer;
int sendStatusState = 0;
boolean isTypeBUSBBoard = false;
boolean firstCharacterReceived = true;
SemaphoreHandle_t xSendDataBLESemaphore;
long updateIntervalTypeBUSBBoard = 50; //50 ms default ?
long lastUpdateTimeTypeBUSBBoard = 0;
int updateModeTypeBUSBBoard = 0;

//led
byte currentLedRow = 0;
byte lastLedPageCol = 0;
byte currentLedCol = 0;
byte currentLedPage = 0;
byte lastLedPage = 0;

//general mode
boolean isPassiveMode = false;

//SPI Bus expander for bus data read & write
MCP23S17 SPIExpander(&SPI, CONTROLLER_OUTPUT_CHIPSELECT, 0);

//SPI Bus expander for buttons and bus control signals
MCP23S17 SPIExpanderButtons(&SPI, CONTROLLER_OUTPUT_CHIPSELECT_BUTTONS, 7);


//Display current led row over the board
void displayLedRow() {

  if (ledsAreDisabled) {
    return;
  }

  //TEST

  //  for (int i = 0; i < 8; i++) {
  //
  //    if (serialLog) Serial.print("LED STATUS LINE: ");
  //    if (serialLog) Serial.print( 8 - (i % 8) , DEC);
  //    if (serialLog) Serial.print(":  ");
  //
  //    for (int j = 0; j < 8; j++) {
  //      if (serialLog) Serial.print(ledStatus[i * 8 + j], DEC);
  //      if (serialLog) Serial.print(" ");
  //    }
  //    if (serialLog) Serial.println("");
  //
  //    if (i % 8 == 7) {
  //      if (serialLog) Serial.println("");
  //    }
  //  }



  if (isPassiveMode) {
    //No led paint if another module is in control
    return;
  }

  byte rowsProcessed = 0;

  while (rowsProcessed < 8) {
    boolean anyLedLit = false;
    rowsProcessed++;

    if (currentLedPage != lastLedPage) {

      currentLedRow++;
      if (currentLedRow > 7) {
        currentLedRow = 0;
      }

      lastLedPage = currentLedPage;
    }

    //check if only one row is lit. If that is the case we will cycle through the columns
    byte numLitRows = 0;
    for (int i = 0; i < 8; i++) {

      boolean anyLedRowLit = false;

      for (int j = 0; j < 8; j++) {
        byte led = (ledStatus[((7 - i) * 8) + 7 - j] == 4);
        if (led) {
          anyLedRowLit = true;
        }
      }
      if (anyLedRowLit) {
        numLitRows++;
      }
    }

    //    if (serialLog) Serial.print("numLitRows: ");
    //    if (serialLog) Serial.println( numLitRows , DEC);


    //get column values
    byte currentLed = 0;
    for (int i = 0; i < 8; i++) {

      byte led = (ledStatus[((7 - currentLedRow) * 8) + 7 - i] == 4);
      if (led) {
        anyLedLit = true;
        if (numLitRows > 1) {
          bitSet(currentLed, i);
        }
      }

    }


    if (anyLedLit && numLitRows == 1) {
      //lit columns alternatively starting from the last lit
      int numCol = currentLedCol;
      if (currentLedPage != lastLedPageCol) {
        numCol++;
        if (numCol > 7) {
          numCol = 0;
        }
        lastLedPageCol = currentLedPage;
      }

      int loops = 0;
      do  {
        loops++;
        byte led = (ledStatus[((7 - currentLedRow) * 8) + 7 - numCol] == 4);
        if (led) {
          bitSet(currentLed, numCol);
          currentLedCol = numCol;
          break;
        }
        numCol ++;
        if (numCol > 7) {
          numCol = 0;
        }
      } while (loops < 8);
    }

    if (!anyLedLit) {
      currentLedRow++;
      if (currentLedRow > 7) {
        currentLedRow = 0;
      }
      continue;
    }


    //write col to latches
    //
    //    if (serialLog)  Serial.print("WRITING LEDS ROW: ");
    //    if (serialLog)  Serial.print(currentLedRow, DEC);
    //    if (serialLog)  Serial.print(" COLS: ");
    //    if (serialLog)  Serial.println(currentLed, BIN);


    SPIExpanderButtons.digitalWrite(COLUMN_DISABLE_EX, true); //disable column to bus

    digitalWrite(CONTROLLER_OUTPUT_DISABLE, false); //enable PIC to bus

    //Load  leds latch
    SPIExpander.writePort(1, currentLed);
    SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, true); //unlatch leds
    SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, false); //latch leds

    //Configure bus for writing to row latch


    //Write row address to expander
    byte encodedRow = 0xFF;
    bitWrite(encodedRow, 7 - currentLedRow, 0);
    SPIExpander.writePort(1, encodedRow);
    SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, true); //open row load latch
    SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, false); //latch row

    //lit leds
    SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, false);

    digitalWrite(CONTROLLER_OUTPUT_DISABLE, true); //disable PIC to bus

    return;
  }
}


//Put the board bus in default mode
void busValuesToDefault() {
  digitalWrite(CONTROLLER_OUTPUT_DISABLE, true);
  SPIExpanderButtons.digitalWrite(LED_WR_DISABLE_COLUMN_LOAD_EX, true);
  SPIExpanderButtons.digitalWrite(LED_WR_LOAD_EX, false);
  SPIExpanderButtons.digitalWrite(ROW_LOAD_EX, false);
  SPIExpanderButtons.digitalWrite(COLUMN_DISABLE_EX, true);
}




//Check for external chess module presence
//We will detect it looking for activity over the 4 board control lines for a couple of seconds
//Force input mode
boolean checkForExternalModule() {

  SPIExpanderButtons.pinMode(LED_WR_DISABLE_COLUMN_LOAD_EX, INPUT);
  SPIExpanderButtons.pinMode(LED_WR_LOAD_EX, INPUT);
  SPIExpanderButtons.pinMode(ROW_LOAD_EX, INPUT);
  SPIExpanderButtons.pinMode(COLUMN_DISABLE_EX, INPUT);

  //ENABLE BOARD BUS
  pinMode(EN_BUS, OUTPUT);
  digitalWrite(EN_BUS, true);

  delay(100);

  int count=0;

  boolean initialState1 = SPIExpanderButtons.digitalRead(LED_WR_DISABLE_COLUMN_LOAD_EX);
  boolean initialState2 = SPIExpanderButtons.digitalRead(LED_WR_LOAD_EX);
  boolean initialState3 = SPIExpanderButtons.digitalRead(ROW_LOAD_EX);
  boolean initialState4 = SPIExpanderButtons.digitalRead(COLUMN_DISABLE_EX);
  
  unsigned long time = millis();

  while (millis() < time + 2000 ) {

    boolean currentState1 = SPIExpanderButtons.digitalRead(LED_WR_DISABLE_COLUMN_LOAD_EX);
    boolean currentState2 = SPIExpanderButtons.digitalRead(LED_WR_LOAD_EX);
    boolean currentState3 = SPIExpanderButtons.digitalRead(ROW_LOAD_EX);
    boolean currentState4 = SPIExpanderButtons.digitalRead(COLUMN_DISABLE_EX);

    if ((initialState1 != currentState1) || (initialState2 != currentState2) || (initialState3 != currentState3) || (initialState4 != currentState4)) {
      
      count++;
      initialState1 = currentState1;
      initialState2 = currentState2;
      initialState3 = currentState3;
      initialState4 = currentState4;
    } 
  }

  if (serialLog) Serial.print("Transitions detected in bus: ");
  if (serialLog) Serial.println(count,DEC);

  if (count > 10) {
    return true;
  }

  return false;
}



//Turn off all leds
void shitchOffLed(byte i) {
  for (int j = 0; j < 8; j++) {
    ledStatusBuffer[63 - i + (64 * j)] = 0;
  }
}



void clearAllBoardLeds() {
  for (int i = 0; i < 64 * 8; i++) {
    ledStatusBuffer[i] = 0;
  }

}
