/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _LICHESS_H_
#define _LICHESS_H_

#include "serial.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include "screen.h"
#include "board.h"
#include "i2c_clock.h"
#include "nvs_conf.h"
#include "soc/rtc_wdt.h" 

const byte OPPONENT_DRAW_OFFER = 0x60;
const byte OPPONENT_TAKEBACK_PROPOSAL = 0x61;


extern byte lichessBoardPosition[64];
extern byte lichessBoardPositionMarked[64];

extern const char* lichessWhiteName;
extern const char* lichessBlackName;
extern const char* lichessWhiteTitle;
extern const char* lichessBlackTitle;
extern long lichessWhiteTime;
extern long lichessBlackTime;
extern long lichessWhiteRating;
extern long lichessBlackRating;

extern byte measuredRssi;

extern boolean isLichessConnected;

extern boolean lichessStreamTvActive;

extern byte* lichessIncomingMovement;

extern byte* lichessOutgoingMovement;

extern char lichessColorToMove;

void initializeWifi();
boolean getRunningGameLichess();
void getIncomingGameEventsLichess();
void refreshLichessClocks();
boolean sendOutgoingMovementLichess(byte originSquare, byte destinationSquare);
void getLichessTvEvents();
void setLichessBoardLeds();
void fixBoardPositionLichessData();
boolean isLichessGameActive();
boolean isLichessGameRunning();
boolean isMyTurnLichess();
void checkRefreshLichessConnections();
void initLichessLoopTask();
void initRefreshLichessConnectionsTask();
void lichessButtonPush(byte button);
void resetLichessGameActions();
boolean isLichessFullyInitialized();

#endif
