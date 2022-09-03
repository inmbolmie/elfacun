/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _BLE_H_
#define _BLE_H_

//BOARD DATA
#include "serial.h"
#include "esp_pins.h"
#include <MCP23S17.h>
#include <vector>

//Piece codes for chess pieces:
#define EMPTY       0x00
#define WPAWN       0x01
#define WROOK       0x02
#define WKNIGHT     0x03
#define WBISHOP     0x04
#define WKING       0x05
#define WQUEEN      0x06
#define BPAWN       0x07
#define BROOK       0x08
#define BKNIGHT     0x09
#define BBISHOP     0x0a
#define BKING       0x0b
#define BQUEEN      0x0c

typedef struct  {
  byte   square;
  byte   oldPiece;
  byte   newPiece;
} SquareChange;

typedef struct  mov {
  byte   originSquare;
  byte   destinationSquare;
  byte   originPiece;
  byte   destinationPiece;
  byte   takenPiece;
  byte   takenPieceSquare;
  struct mov *previousMovement;
  struct mov *nextMovement;
} Movement;

extern Movement *lastMovement;

extern boolean boardIsOutOfSync;

extern SquareChange changes[100];
extern int numChanges;

extern byte initpos[64];

extern byte lastDrawn[64];

extern byte lastDrawnBG[64];

extern boolean lastpos[64];

extern boolean scannedpos[64];

extern boolean transposescannedpos[64];

extern boolean passivescannedpos[64];


extern byte ledStatusBuffer[8 * 64];
extern byte ledStatus[64];
extern byte ledInterval;
extern boolean isModeALeds;
extern boolean modeAautoLedsOff;
extern boolean modeAautoReverseLeds;
extern boolean ledUpdateReceived ;
extern int millisLed;
extern int ledIntervalCycles;
extern boolean ledsAreDisabled;


extern byte liftedPiece;
extern byte liftedPieceSquare ;
extern byte liftedPiece2 ;
extern byte liftedPiece2Square;
extern byte promotingSquare;
extern boolean boardInInitialPos;
extern boolean settingUpBoardMode;

extern boolean takingBackMovement;

extern byte currentPos[64];

extern boolean boardInverted ;


//BLE objects

extern boolean BLEdeviceConnected;
extern std::vector<byte> ledbuffer;
extern int sendStatusState ;
extern boolean isTypeBUSBBoard ;
extern boolean firstCharacterReceived ;
extern SemaphoreHandle_t xSendDataBLESemaphore;
extern long updateIntervalTypeBUSBBoard ;
extern long lastUpdateTimeTypeBUSBBoard ;
extern int updateModeTypeBUSBBoard;
extern boolean sendFullUpdateOnNextInitialPos;

//led
extern byte currentLedRow;
extern byte lastLedPageCol ;
extern byte currentLedCol ;
extern byte currentLedPage ;
extern byte lastLedPage ;

//general mode
extern boolean isPassiveMode;

//SPI Bus expander for bus data read & write
extern MCP23S17 SPIExpander;

//SPI Bus expander for buttons and bus control signals
extern MCP23S17 SPIExpanderButtons;




void displayLedRow();
void busValuesToDefault();
boolean checkForExternalModule();
void read_module_buttons();
void shitchOffLed(byte i);
void clearAllBoardLeds();

#endif
