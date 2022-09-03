/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#ifndef _SCREEN_H_
#define _SCREEN_H_

//SPI SCREEN MANAGEMENT ROUTINES

#include <TFT_eSPI.h>
#include "clock.h"
#include "board.h"
#include "board_a.h"
#include "Free_Fonts.h" 


extern TFT_eSPI tft;  

void drawBmp(const char *filename, int16_t x, int16_t y);
void display_clock();
void display_message(String message);
void clear_message();
void displayBoard();
void initialize_tft();

void paintLichessPlayerTop(const char* lichessTopTitle,const char*  lichessTopName,long  lichessTopRating);
void paintLichessPlayerTopSprite(const char* lichessTopTitle,const char*  lichessTopName,long  lichessTopRating);
void refreshLichessPlayerTopSprite();

void paintLichessPlayerBottom(const char* lichessBottomTitle,const char*  lichessBottomName,long  lichessBottomRating);
void paintLichessPlayerBottomSprite(const char* lichessTopTitle,const char*  lichessTopName,long  lichessTopRating);
void refreshLichessPlayerBottomSprite();

void paintLichessPlayerClear();

void paintLichessTimeTop(long lichessTopTime, boolean isTimeToMove);
void paintLichessTimeBottom(long lichessBottomTime, boolean isTimeToMove);

void askForLichessConfirmation(const char* lichessTime, const char* lichessIncrement, const char* lichessRated, const char* lichessColor);
void cancelLichessConfirmation();

void cancelLichessSeekScreen();
void showLichessSeekScreen();

void askForLichessConfirmationDrawOffer();
void askForLichessConfirmationResign();
void askForLichessConfirmationTakeBackOffer();

void askForLichessConfirmationOpponentDrawOffer();
void askForLichessConfirmationOpponentTakebackProposal();

void paintLichessGameEndedDraw();
void paintLichessGameWonBottom();
void paintLichessGameWonTop();

void displayLichessLogo();
void displayModeALogo();
void displayModeBLogo();

#endif
