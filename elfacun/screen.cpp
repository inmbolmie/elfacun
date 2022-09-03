/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "screen.h"
#include "serial.h"
#include "i2c_clock.h"
#include "lichess.h"

int lichessTopLastShownTime = 0;
int lichessBottomLastShownTime = 0;
boolean lichessBottomLastShownTimeToMove = false;
boolean lichessTopLastShownTimeToMove = false;

char lichessConfirmationMsg[200];
boolean displayLichessConfirmationMsg = false;
boolean displayLichessSeekMsg = false;
const char*  lichessBottomNameActive;
const char*  lichessTopNameActive;
boolean signalResetTopPlayer = false;
boolean signalResetBottomPlayer = false;
int topPlayerTextSize = 0;
int bottomPlayerTextSize = 0;



int toScrollTopPlayer = 0;
int toScrollBottomPlayer = 0;

TFT_eSPI tft = TFT_eSPI();
TFT_eSprite spr = TFT_eSprite(&tft);
TFT_eSprite sprb = TFT_eSprite(&tft);


void initialize_tft() {
  tft.init();            // initialize LCD
  tft.setRotation(3);    //3-Landscape mode with SD card slot pointing backwards
  tft.setSwapBytes(true);
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(TFT_WHITE); tft.setTextSize(1);
  
}


// These read 16- and 32-bit types from the SD card file.
// BMP data is stored little-endian, Arduino is little-endian too.
// May need to reverse subscript order if porting elsewhere.

uint16_t read16(fs::File &f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

uint32_t read32(fs::File &f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}


// Bodmers BMP image rendering function
// https://github.com/Bodmer/TFT_eSPI/blob/master/examples/Generic/TFT_SPIFFS_BMP/BMP_functions.ino
void drawBmp(const char *filename, int16_t x, int16_t y) {

  if ((x >= tft.width()) || (y >= tft.height())) return;

  fs::File bmpFS;

  // Open requested file on SD card
  bmpFS = SPIFFS.open(filename, "r");

  if (!bmpFS)
  {
    if (serialLog) Serial.print("File not found: ");
    if (serialLog) Serial.println(filename);
    return;
  }

  uint32_t seekOffset;
  uint16_t w, h, row, col;
  uint8_t  r, g, b;

  uint32_t startTime = millis();

  if (read16(bmpFS) == 0x4D42)
  {
    read32(bmpFS);
    read32(bmpFS);
    seekOffset = read32(bmpFS);
    read32(bmpFS);
    w = read32(bmpFS);
    h = read32(bmpFS);

    if ((read16(bmpFS) == 1) && (read16(bmpFS) == 24) && (read32(bmpFS) == 0))
    {
      y += h - 1;

      bool oldSwapBytes = tft.getSwapBytes();
      tft.setSwapBytes(true);
      bmpFS.seek(seekOffset);

      uint16_t padding = (4 - ((w * 3) & 3)) & 3;
      uint8_t lineBuffer[w * 3 + padding];

      for (row = 0; row < h; row++) {

        bmpFS.read(lineBuffer, sizeof(lineBuffer));
        uint8_t*  bptr = lineBuffer;
        uint16_t* tptr = (uint16_t*)lineBuffer;
        // Convert 24 to 16 bit colours
        for (uint16_t col = 0; col < w; col++)
        {
          b = *bptr++;
          g = *bptr++;
          r = *bptr++;
          *tptr++ = ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
        }

        // Push the pixel row to screen, pushImage will crop the line if needed
        // y is decremented as the BMP image is drawn bottom up
        tft.pushImage(x, y--, w, 1, (uint16_t*)lineBuffer);
      }
      tft.setSwapBytes(oldSwapBytes);
      //if (serialLog) Serial.print("Loaded in ");
      //if (serialLog) Serial.print(millis() - startTime);
      //if (serialLog) Serial.println(" ms");
    }
    else if (serialLog) Serial.println("BMP format not recognized.");
  }
  bmpFS.close();
}


//
//void displayLichessConfirmationMessage() {
//
//  if (displayLichessConfirmationMsg) {
//    spr.pushSprite(0, 60);
//    if (serialLog) Serial.println("Push sprite------------------------------------>");
//  }
//}



const char hex[17] = "0123456789ABCDEF";

void displayBoard() {

  int boardOriginX = 0;
  int boardOriginY = 0;

  if ((isExternalClock || isTypeBUSBBoard) && !isLichessConnected) {
    boardOriginX = 40;
    boardOriginY = 0;
  }

  tft.setTextSize(2);
  tft.setCursor(boardOriginX, boardOriginY);



  int square = 0;
  int visibleSquare = 0;
  int line = 0;
  int linepos = 0;

  int x = boardOriginX;
  int y = boardOriginY;

  if (!isExternalClock && refreshClock) {
    display_clock();
    send_clock();
    refreshClock = false;
  }



  while (square < 64) {

    visibleSquare = square;

    byte bgcolor = 1;
    if ((line % 2) == (square % 2)) {
      bgcolor = 0;
    }

    x = boardOriginX + (square % 8) * 30;
    y = boardOriginY + line * 30;

    byte squareToShow = currentPos[visibleSquare];

    if (boardInverted) {
      squareToShow = currentPos[63 - visibleSquare];
    }

    if (isLichessConnected) {
      int index = visibleSquare;
      if (boardInverted) {
        index = 63 - visibleSquare;
      }
      squareToShow = lichessBoardPosition[index];
      if (lichessBoardPositionMarked[index] == 1) {
        bgcolor += 2;
      }
    }


    if ((lastDrawn[visibleSquare] != squareToShow || lastDrawnBG[visibleSquare] != bgcolor)  && ((displayLichessConfirmationMsg == false && displayLichessSeekMsg == false) || square <= 15 || square >= 48  )) {
      lastDrawn[visibleSquare] = squareToShow;
      lastDrawnBG[visibleSquare] = bgcolor;
      String pieceChar = String(squareToShow, HEX);
      pieceChar.toUpperCase();
      String filename = "/" + pieceChar + bgcolor + ".bmp";
      drawBmp(filename.c_str(), x, y);


    }

    square++;
    linepos++;
    if (linepos == 8) {
      linepos = 0;
      line++;
      tft.println("");
    }
  }

  //displayLichessConfirmationMessage();


}



void displayLichessLogo() {
  if (serialLog) Serial.println("Displaying Lichess logo");
  tft.fillScreen(TFT_BLACK);
  drawBmp("/lich.bmp", 110, 70);  
}



void displayModeALogo(){
  if (serialLog) Serial.println("Displaying Mode A logo");
  tft.fillScreen(TFT_BLACK);
  drawBmp("/modea.bmp", 110, 100);  
}


void displayModeBLogo(){
  if (serialLog) Serial.println("Displaying Mode B logo");
  tft.fillScreen(TFT_BLACK);
  drawBmp("/modeb.bmp", 110, 100);  
}


void clear_message() {
  tft.fillRect(245, 110, 75, 30, TFT_BLACK);
}


void display_message(String message) {
  tft.fillRect(245, 115, 75, 30, TFT_BLACK);
  tft.setCursor(245, 115);
  tft.setTextColor(TFT_WHITE);
  tft.println(message);

}


void display_clock() {


  if (!boardInverted) {

    tft.fillRect(245, 30, 75, 50, TFT_BLACK);
    tft.setCursor(255, 50);
    tft.setTextColor(TFT_WHITE);
    tft.printf("%01d", rhours);
    tft.print(":");
    tft.printf("%02d", rmins);
    tft.print(".");
    tft.printf("%02d", rsecs);

    tft.fillRect(245, 180, 75, 30, TFT_BLACK);
    tft.setCursor(255, 180);
    tft.printf("%01d", lhours);
    tft.print(":");
    tft.printf("%02d", lmins);
    tft.print(".");
    tft.printf("%02d", lsecs);
  } else {

    tft.fillRect(245, 30, 75, 50, TFT_BLACK);
    tft.setCursor(255, 50);
    tft.setTextColor(TFT_WHITE);
    tft.printf("%01d", lhours);
    tft.print(":");
    tft.printf("%02d", lmins);
    tft.print(".");
    tft.printf("%02d", lsecs);
    
    tft.fillRect(245, 180, 75, 30, TFT_BLACK);
    tft.setCursor(255, 180);
    tft.printf("%01d", rhours);
    tft.print(":");
    tft.printf("%02d", rmins);
    tft.print(".");
    tft.printf("%02d", rsecs);
  }
}



void paintLichessPlayerTopSprite(const char* lichessTopTitle, const char*  lichessTopName, long  lichessTopRating) {

  tft.fillRect(240, 0, 80, 80, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  if (isLichessConnected && !isLichessGameActive()) {
    tft.setCursor(245, 0);
    tft.setTextColor(TFT_RED);
    tft.printf("LichessTV");
  }

  tft.setCursor(247, 25);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("%s", lichessTopTitle);


  //tft.setCursor(247, 45);
  //tft.setTextColor(TFT_WHITE);
  //tft.printf("%s", lichessTopName);


  if (lichessTopRating != NULL) {
    tft.setCursor(247, 65);
    tft.setTextColor(TFT_DARKGREY);
    tft.printf("%d", lichessTopRating);
  }



  spr.setColorDepth(8);
  spr.deleteSprite();
  spr.loadFont("NotoSansBold15"); // Must load the font first into the sprite class
  topPlayerTextSize = spr.textWidth(lichessTopName) + 7;
  spr.createSprite(topPlayerTextSize, 17);
  //spr.fillSprite(TFT_RED);

  spr.setTextColor(TFT_WHITE, TFT_BLACK); // Set the sprite font colour and the background colour
  //tft.setCursor(247, 45);
  //spr.setScrollRect(0, 0, 10 * strlen(lichessTopName), 17, TFT_BLACK);
  spr.printToSprite(lichessTopName);    // Prints to tft cursor position, tft cursor NOT moved

  spr.unloadFont(); // Remove the font from sprite class to recover memory used
  tft.fillRect(247, 45, 80, 20, TFT_BLACK);
  spr.pushSprite(247, 45);
  if (serialLog) Serial.print("Sprite TOP width----------------------> ");
  if (serialLog) Serial.println(topPlayerTextSize, DEC);


  toScrollTopPlayer = topPlayerTextSize + 60;
  delete lichessTopNameActive;
  lichessTopNameActive = strdup(lichessTopName);

  signalResetTopPlayer = false;

  return;

}


void refreshLichessPlayerTopSprite() {

  if (spr.width() > 0) {

    toScrollTopPlayer--;

    if ((toScrollTopPlayer >= 80) && (toScrollTopPlayer <= topPlayerTextSize) ) {
      spr.scroll(-1);
      spr.pushSprite(247, 45);
    }

    if (toScrollTopPlayer < 20) {
      signalResetTopPlayer = true;
    }

    if (toScrollTopPlayer < 20 && signalResetBottomPlayer) {
      signalResetBottomPlayer = false;
      spr.deleteSprite();
      spr.createSprite(topPlayerTextSize, 17);
      spr.loadFont("NotoSansBold15");
      //tft.fillRect(247, 45, 80, 20, TFT_BLACK);
      spr.fillSprite(TFT_BLACK);
      tft.setCursor(247, 45);
      spr.printToSprite(lichessTopNameActive);
      toScrollTopPlayer = topPlayerTextSize + 60;
      spr.unloadFont();
      spr.pushSprite(247, 45);

    }
  }
}



void paintLichessPlayerBottomSprite(const char* lichessBottomTitle, const char*  lichessBottomName, long  lichessBottomRating) {


  tft.fillRect(240, 120, 80, 80, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  if (lichessBottomRating != NULL) {
    tft.setCursor(247, 160);
    tft.setTextColor(TFT_DARKGREY);
    tft.printf("%d", lichessBottomRating);
  }

  //tft.setCursor(247, 180);
  //tft.setTextColor(TFT_WHITE);
  //tft.printf("%s", lichessBottomName);


  tft.setCursor(247, 200);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("%s", lichessBottomTitle);


  sprb.setColorDepth(8);
  sprb.deleteSprite();
  sprb.loadFont("NotoSansBold15"); // Must load the font first into the sprite class
  bottomPlayerTextSize = sprb.textWidth(lichessBottomName) + 7;
  sprb.createSprite(bottomPlayerTextSize, 17);
  //spr.fillSprite(TFT_RED);

  sprb.setTextColor(TFT_WHITE, TFT_BLACK); // Set the sprite font colour and the background colour
  //tft.setCursor(247, 45);
  //spr.setScrollRect(0, 0, 10 * strlen(lichessTopName), 17, TFT_BLACK);
  sprb.printToSprite(lichessBottomName);    // Prints to tft cursor position, tft cursor NOT moved

  sprb.unloadFont(); // Remove the font from sprite class to recover memory used
  tft.fillRect(247, 180, 80, 20, TFT_BLACK);
  sprb.pushSprite(247, 180);

  if (serialLog) Serial.print("Sprite BOTTOM width----------------------> ");
  if (serialLog) Serial.println(bottomPlayerTextSize, DEC);

  toScrollBottomPlayer = bottomPlayerTextSize + 60;
  delete lichessBottomNameActive;
  lichessBottomNameActive = strdup(lichessBottomName);

  signalResetBottomPlayer = false;


  return;
}



void refreshLichessPlayerBottomSprite() {

  if (sprb.width() > 0) {

    toScrollBottomPlayer--;

    if ((toScrollBottomPlayer >= 80) && (toScrollBottomPlayer <= bottomPlayerTextSize )) {
      sprb.scroll(-1);
      sprb.pushSprite(247, 180);
    }

    if (toScrollBottomPlayer < 20) {
      signalResetBottomPlayer = true;
    }

    if (toScrollBottomPlayer < 20 && signalResetTopPlayer) {
      signalResetTopPlayer = false;
      sprb.deleteSprite();
      sprb.createSprite(bottomPlayerTextSize, 17);
      sprb.loadFont("NotoSansBold15");
      //tft.fillRect(247, 180, 80, 20, TFT_BLACK);
      sprb.fillSprite(TFT_BLACK);
      tft.setCursor(247, 180);
      sprb.printToSprite(lichessBottomNameActive);
      toScrollBottomPlayer = bottomPlayerTextSize + 60;
      sprb.unloadFont();
      sprb.pushSprite(247, 180);

    }
  }
}




void paintLichessPlayerTop(const char* lichessTopTitle, const char*  lichessTopName, long  lichessTopRating) {

  tft.fillRect(240, 0, 80, 80, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  if (isLichessConnected && !isLichessGameActive()) {
    tft.setCursor(245, 0);
    tft.setTextColor(TFT_RED);
    tft.printf("LichessTV");
  }

  tft.setCursor(247, 25);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("%s", lichessTopTitle);


  tft.setCursor(247, 45);
  tft.setTextColor(TFT_WHITE);
  tft.printf("%s", lichessTopName);


  if (lichessTopRating != NULL) {
    tft.setCursor(247, 65);
    tft.setTextColor(TFT_DARKGREY);
    tft.printf("%d", lichessTopRating);
  }

  return;
}

void paintLichessPlayerBottom(const char* lichessBottomTitle, const char*  lichessBottomName, long  lichessBottomRating) {


  tft.fillRect(240, 120, 80, 80, TFT_BLACK);
  tft.setTextSize(1);
  tft.setTextWrap(false);

  if (lichessBottomRating != NULL) {
    tft.setCursor(247, 160);
    tft.setTextColor(TFT_DARKGREY);
    tft.printf("%d", lichessBottomRating);
  }

  tft.setCursor(247, 180);
  tft.setTextColor(TFT_WHITE);
  tft.printf("%s", lichessBottomName);


  tft.setCursor(247, 200);
  tft.setTextColor(TFT_ORANGE);
  tft.printf("%s", lichessBottomTitle);



  return;
}

void paintLichessPlayerClear() {
  tft.fillRect(240, 0, 80, 240, TFT_BLACK);
  return;
}

void paintLichessTimeTop(long lichessTopTime, boolean isTimeToMove) {
  if (lichessTopTime != NULL) {

    if (lichessTopTime / 1000 == lichessTopLastShownTime && isTimeToMove == lichessTopLastShownTimeToMove) {
      return;
    }

    lichessTopLastShownTime = lichessTopTime / 1000;
    lichessTopLastShownTimeToMove = isTimeToMove;

    long minutes = lichessTopTime / 60000;
    long seconds = (lichessTopTime / 1000) % 60;

    tft.fillRect(247, 95, 80, 20, TFT_BLACK);

    tft.setCursor(260, 95);
    if (!isTimeToMove) {
      tft.setTextColor(TFT_DARKGREY);
    } else {
      tft.setTextColor(0xFA80);
    }
    tft.printf("%d:%02d", minutes, seconds);
  }
  return;
}

void paintLichessTimeBottom(long lichessBottomTime, boolean isTimeToMove) {
  if (lichessBottomTime != NULL) {

    if (lichessBottomTime / 1000 == lichessBottomLastShownTime && isTimeToMove == lichessBottomLastShownTimeToMove) {
      return;
    }

    lichessBottomLastShownTime = lichessBottomTime / 1000;
    lichessBottomLastShownTimeToMove = isTimeToMove;

    long minutes = lichessBottomTime / 60000;
    long seconds = (lichessBottomTime / 1000) % 60;

    tft.fillRect(247, 125, 80, 20, TFT_BLACK);

    tft.setCursor(260, 125);
    if (!isTimeToMove) {
      tft.setTextColor(TFT_DARKGREY);
    } else {
      tft.setTextColor(0xFA80);
    }
    tft.printf("%d:%02d", minutes, seconds);
  }
  return;

}


void askForLichessConfirmation(const char* lichessTime, const char* lichessIncrement, const char* lichessRated, const char* lichessColor) {

  displayLichessConfirmationMsg = true;

  //spr.setColorDepth(8);
  //spr.createSprite(240, 120);
  //spr.fillSprite(TFT_RED);
  //spr.loadFont("NotoSansBold15"); // Must load the font first into the sprite class
  //spr.setTextColor(TFT_WHITE, TFT_BLACK); // Set the sprite font colour and the background colour
  //tft.setCursor(0,0);
  //spr.setScrollRect(0, 0, 200, 80, TFT_YELLOW);
  //spr.printToSprite(lichessConfirmationMsg);    // Prints to tft cursor position, tft cursor NOT moved
  //spr.unloadFont(); // Remove the font from sprite class to recover memory used

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 63);
  tft.setTextColor(TFT_WHITE);
  tft.printf("Push again to launch seek for:\nTime: %s Increment: %s\nColor: %s\nRated: %s\nVariant: standard\n\nPress other key to cancel", lichessTime, lichessIncrement, lichessColor, lichessRated );

}



void askForLichessConfirmationTakeBackOffer() {

  displayLichessConfirmationMsg = true;

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 83);
  tft.setTextColor(TFT_WHITE);
  tft.println(" Press again to ask takeback");
  tft.println("");
  tft.println(" Push any other button to abort");

}



void askForLichessConfirmationDrawOffer() {

  displayLichessConfirmationMsg = true;

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 83);
  tft.setTextColor(TFT_WHITE);
  tft.println(" Press again to offer draw");
  tft.println("");
  tft.println(" Push any other button to abort");

}

void askForLichessConfirmationResign() {

  displayLichessConfirmationMsg = true;

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 83);
  tft.setTextColor(TFT_WHITE);
  tft.println(" Press again to resign");
  tft.println("");
  tft.println(" Push any other button to abort");

}


void askForLichessConfirmationOpponentDrawOffer() {

  displayLichessConfirmationMsg = true;

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 83);
  tft.setTextColor(TFT_WHITE);
  tft.println(" Opponent offered a draw");
  tft.println("");
  tft.println(" Push first button to accept");
  tft.println("");
  tft.println(" Any other to decline");

}


void askForLichessConfirmationOpponentTakebackProposal() {

  displayLichessConfirmationMsg = true;

  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(0, 83);
  tft.setTextColor(TFT_WHITE);
  tft.println(" Opponent ask taking back");
  tft.println("");
  tft.println(" Push first button to accept");
  tft.println("");
  tft.println(" Any other to decline");

}




void cancelLichessConfirmation() {
  if (displayLichessConfirmationMsg) {
    displayLichessConfirmationMsg = false;
    memset(lastDrawn, 255, 64);
    memset(lastDrawnBG, 255, 64);
  }

}

void cancelLichessSeekScreen() {
  displayLichessSeekMsg = false;
  memset(lastDrawn, 255, 64);
  memset(lastDrawnBG, 255, 64);
}

void showLichessSeekScreen() {
  displayLichessSeekMsg = true;
  tft.fillRect(0, 60, 240, 120, TFT_BLACK);
  tft.setCursor(15, 90);
  tft.setTextColor(TFT_WHITE);
  tft.printf("Searching for an opponent...");

}



void paintLichessGameEndedDraw() {

  tft.fillRect(247, 95, 80, 20, TFT_BLACK);
  tft.setCursor(260, 95);
  tft.setTextColor(TFT_DARKGREY);
  tft.print("1/2");

  tft.fillRect(247, 125, 80, 20, TFT_BLACK);
  tft.setCursor(260, 125);
  tft.setTextColor(TFT_DARKGREY);
  tft.print("1/2");

  if (serialLog) Serial.println("-------------------->GAME ENDED DRAW");

}

void paintLichessGameWonBottom() {

  tft.fillRect(247, 95, 80, 20, TFT_BLACK);
  tft.setCursor(260, 95);
  tft.setTextColor(TFT_RED);
  tft.print("0");

  tft.fillRect(247, 125, 80, 20, TFT_BLACK);
  tft.setCursor(260, 125);
  tft.setTextColor(TFT_GREEN);
  tft.print("1");

  if (serialLog) Serial.println("-------------------->GAME ENDED BOTTOM");

}

void paintLichessGameWonTop() {

  tft.fillRect(247, 95, 80, 20, TFT_BLACK);
  tft.setCursor(260, 95);
  tft.setTextColor(TFT_GREEN);
  tft.print("1");

  tft.fillRect(247, 125, 80, 20, TFT_BLACK);
  tft.setCursor(260, 125);
  tft.setTextColor(TFT_RED);
  tft.print("0");

  if (serialLog) Serial.println("-------------------->GAME ENDED TOP");

}
