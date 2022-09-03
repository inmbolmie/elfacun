/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "lichess.h"

//Built over:
/*
  Lichess - link V.1.0.0. - ALPHA
  This sketch connects an Arduino Uno Wifi Rev2 to the lichess api
  and allows you to play a correspondance or random opponent chess game
  on your arduino using a LCD display shield with input buttons.
  Please see Github page for more thorough documentation :
  https://github.com/Kzra/Lichess-Link
  Acknowledgements:
  Several chunks of code were repurposed from
  the WiFiNiNA SSLClient example
  and from the Jsonhttpclient Arduino Json example.
  Copyright MIT (C) Ezra Kitson 2021.
*/



byte lichessBoardPosition[64] =  {8, 9, 10, 12, 11, 10, 9, 8,
                                  7, 7, 7, 7, 7, 7, 7, 7,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  0, 0, 0, 0, 0, 0, 0, 0,
                                  1, 1, 1, 1, 1, 1, 1, 1,
                                  2, 3, 4, 6, 5, 4, 3, 2
                                 };


byte lichessBoardPositionMarked[64] = {0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0,
                                       0, 0, 0, 0, 0, 0, 0, 0
                                      };

//boolean isLichessBoardReversed = false;

//user input variables and constants
char uci[] = {' ', ' ', ' ', ' ', 0};
boolean selectLichess = false;
int coordIndex = 0;
int firstCoord = 0;
int secondCoord = 0;
int thirdCoord = 0;
int fourthCoord = 0;
int alphaNumIndex[37] = { 0 };
int clockTime = 120;
int clockIncrement = 30;
int colourIndex = 0;
char corrUser[16] = {'\0'};
const char columns[] = {' ', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'};
const char rows[] = {' ', '1', '2', '3', '4', '5', '6', '7', '8'};
const char alphaNumeric[] = {'\0', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9'};


char server[] = "lichess.org";    // name address for lichess (using DNS)


//lichess variables
const char* username;
const char* currentGameID;
const char* previousGameID;
const char* lastMove;
const char* myColour = NULL;
const char* opponentName;
const char* opponentColour;
const char* moveError;
const char* winner;
const char* endStatus;
const char* challengeID;
const char* preMove;
const char* selectedColour;
const char* currentFen;
boolean myTurn = false;
boolean moveSuccess = false;
boolean gameInit = true;
boolean challengeSent = false;
boolean isCurrentGameRunning = false;
const int delayTime = 1000; //the time to wait for lichess.org to respond to an http request

// loop variables
boolean MODE0 = true;
boolean MODE1 = false;
int modeIndex = 0;




long lastTimeCommandConnectionUse = 0;
long lastTimeStreamConnectionUse = 0;

WiFiClientSecure wifiClient;
WiFiClientSecure wifiClientStream;

StaticJsonDocument<2048> doc;

//The connection is already initialized for status updates
boolean wifiClientInitializedStatus = false;
boolean wifiClientStreamInitializedStatus = false;


boolean isLichessConnected = false;
boolean lichessStreamTvActive = false;
boolean isLichessSeekActive = false;
boolean refreshLichessConnectionsTaskEnabled = false;
byte secondLichessButtonPush = 0;
byte pendingLichessActionConfirmation = 0;
byte pendingLichessOpponentRequest = 0;

const char* lichessWhiteName = "";
const char* lichessBlackName = "";
const char* lichessWhiteTitle = "";
const char* lichessBlackTitle = "";
long lichessWhiteTime = NULL;
long lichessBlackTime = NULL;
long lichessWhiteRating = NULL;
long lichessBlackRating = NULL;
char lichessColorToMove = 'w';

const TickType_t lichessSemaphoreTimeout = 5000 / portTICK_PERIOD_MS;

SemaphoreHandle_t xCommandConnectionSemaphore;
SemaphoreHandle_t xLichessClockSemaphore;
hw_timer_t * timerLichessClock = NULL;

char* getStatusLineFromClient() {

  char* emptyLine = new char[100];
  memset(emptyLine, 0, 100);

  char* sizeLine = new char[100];
  memset(sizeLine, 0, 100);

  char* line = new char[1000];
  memset(line, 0, 1000);


  if (serialLog) Serial.print("MEM available: ");
  if (serialLog) Serial.println(heap_caps_get_free_size(MALLOC_CAP_8BIT), DEC);

  if (serialLog) Serial.print("BLOCK available: ");
  if (serialLog) Serial.println(heap_caps_get_largest_free_block(MALLOC_CAP_8BIT), DEC);


  if (!wifiClientStream.available()) {
    if (serialLog) Serial.print("Line not available");
    delete emptyLine;
    delete sizeLine;
    return line;
  }

  if (serialLog) Serial.print("Line available: ");
  if (serialLog) Serial.println(wifiClientStream.available(), DEC);

  sizeLine[0] = 0;
  wifiClientStream.readBytesUntil('\n', sizeLine, 100);
  //if (serialLog) Serial.println("Size line: ");
  //if (serialLog) Serial.println(sizeLine);

  line[0] = 0;
  wifiClientStream.readBytesUntil('\n', line, 1000);
  //if (serialLog) Serial.println("Line: ");
  //if (serialLog) Serial.println(line);

  emptyLine[0] = 0;
  wifiClientStream.readBytesUntil('\n', emptyLine, 100);
  //if (serialLog) Serial.println("Empty line line: ");
  //if (serialLog) Serial.println(emptyLine);

  delete emptyLine;
  delete sizeLine;

  return line;

}




char* getLineFromClient(WiFiClientSecure* client) {

  char* line = new char[1000];
  memset(line, 0, 1000);

  line[0] = 0;
  client->readBytesUntil('\n', line, 1000);
  //if (serialLog) Serial.println("Line: ");
  //if (serialLog) Serial.println(line);


  return line;

}


int processHTTP(WiFiClientSecure* client) {
  // Check HTTP status
  char status[320] = {0};

  int returnCode = 200;

  int timeout = millis() + 2000;
  while (client->available() == 0) {
    if (timeout - millis() < 0) {
      if (serialLog) Serial.println("Client Timeout processHTTP");
      client->stop();
      return 500;
    }
  }

  client->readBytesUntil('\r', status, sizeof(status));
  // It should be "HTTP/1.0 200 OK"
  if (strcmp(status + 9, "200 OK") != 0) {
    returnCode = 500;
    if (serialLog) Serial.print("Unexpected response: ");
    if (serialLog) Serial.println(status);
    if (strcmp(status + 9, "400 Bad Request") == 0) {
      if (serialLog) Serial.println("Bad Request!");
      returnCode = 400;
    } else {
      client->stop();
      returnCode = 500;
    }

  }

  // Skip HTTP headers
  char endOfHeaders[] = "\r\n\r\n";
  if (!client->find(endOfHeaders)) {
    if (serialLog) Serial.println("Invalid response");
    returnCode = 500;
  }


  return returnCode;
}



portMUX_TYPE timerMuxLichess = portMUX_INITIALIZER_UNLOCKED;
void IRAM_ATTR onTimerLichess() {

  portENTER_CRITICAL_ISR(&timerMuxLichess);

  if (myColour != NULL && lichessWhiteTime != NULL && lichessBlackTime != NULL) {

    if ( xSemaphoreTake( xLichessClockSemaphore, ( TickType_t ) 0 ) == pdTRUE )
    {

      if (isLichessGameActive() || lichessStreamTvActive) {

        if ((myTurn && !boardInverted) || (!myTurn && boardInverted)) {
          lichessWhiteTime = lichessWhiteTime - 50L;
          if (lichessWhiteTime < 0L) {
            lichessWhiteTime = 0L;
          }
        } else {
          lichessBlackTime = lichessBlackTime - 50L ;
          if (lichessBlackTime < 0L) {
            lichessBlackTime = 0L;
          }
        }

      }

      xSemaphoreGive( xLichessClockSemaphore );
    }
  }

  portEXIT_CRITICAL_ISR(&timerMuxLichess);
}





void initializeWifi() {

  //Semaphore to handle the commands persistent connection
  xCommandConnectionSemaphore = xSemaphoreCreateMutex();

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );


  //Semaphore to handle clocks
  xLichessClockSemaphore = xSemaphoreCreateMutex();
  //unlocked by default
  xSemaphoreGive( ( xLichessClockSemaphore ) );

  //Clock timer
  // Use 3rd timer of 4 (counted from zero).
  // Set 80 divider for prescaler (see ESP32 Technical Reference Manual for more
  // info).
  timerLichessClock = timerBegin(2, 80, true);

  // Attach onTimer function to our timers
  timerAttachInterrupt(timerLichessClock, &onTimerLichess, true);


  // Set alarm to call onTimer function every 50ms (value in microseconds).
  // Repeat the alarm (third parameter)
  timerAlarmWrite(timerLichessClock, 50000, true);

  // Start an alarm
  timerAlarmEnable(timerLichessClock);


  tft.setCursor(0, 0);

  tft.print("Connecting to WIFI...");
  delay(500);
  WiFi.begin(ssid, password);
  delay(1000);

  int numtries = 20;
  
  while (WiFi.status() != WL_CONNECTED && numtries-- > 0 ) {
    delay(500);
    if (serialLog) Serial.println(".");
  }


  if (WiFi.status() != WL_CONNECTED ) {
    if (serialLog) Serial.print("WIFI connection error,: ");
    if (serialLog) Serial.println(WiFi.status(), DEC);
    tft.print("WIFI connection error: ");
    tft.println(WiFi.status(), DEC);
    // WL_IDLE_STATUS = 0, WL_NO_SSID_AVAIL = 1, WL_SCAN_COMPLETED = 2, WL_CONNECTED = 3, WL_CONNECT_FAILED = 4, WL_CONNECTION_LOST = 5, WL_DISCONNECTED = 6
    xSemaphoreGive( ( xCommandConnectionSemaphore ) );

    if (serialLog) Serial.println("Restarting ESP32 in 5 seconds");
    tft.print("Restarting ESP32 in 5 seconds");

    delay(5000);
    ESP.restart();
    return;
  }

  tft.println("");
  tft.println("WiFi connected");
  tft.print("IP address: ");
  tft.println(WiFi.localIP());

  if (serialLog) Serial.println("");
  if (serialLog) Serial.println("WiFi connected");
  if (serialLog) Serial.print("IP address: ");
  if (serialLog) Serial.println(WiFi.localIP());


  tft.println("\nStarting connection to server......");
  // if you get a connection, report back via serial:

  wifiClient.setInsecure();
  wifiClient.setTimeout(10);

  wifiClientStream.setInsecure();
  wifiClientStream.setTimeout(10);

  if (wifiClient.connect(server, 443)) {
    tft.println("connected to server in setup");
    // SETUP API: MAKE A REQUEST TO DOWNLOAD THE CURRENT USER'S LICHESS USERNAME
    wifiClient.println("GET /api/account HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    // Include an authorisation header with the lichess API token
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();
    //delay(1000); //delay to allow a response

    //tft.print("process HTTP headers... ");
    processHTTP(&wifiClient);
    //delay(1000);
    //tft.print("processed HTTP headers... ");
    // Allocate the JSON document
    // Use arduinojson.org/v6/assistant to compute the capacity.


    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout initializeWifi");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }
    }

    //DynamicJsonDocument doc(1024);
    // Parse JSON object
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error) {
      // this is due to an error in the HTTP request
      tft.print(F("deserializeJson() failed: "));
      tft.println(error.f_str());
      tft.print("HTTP failed... ");
      tft.print("On Setup ");
      delay(1000);
      xSemaphoreGive( ( xCommandConnectionSemaphore ) );
      return;
    }
    // Extract values
    //tft.println(F("Response:"));
    // lichess username
    delete username;
    username = strdup(doc["username"]);
    //tft.println(username);
    //close request
    //client.println("Connection: close");
    //client.println();
    if (username != NULL) {

      tft.println("Connected to Lichess!");
      tft.print("User: ");
      tft.println(username);
    }
    //we were unable to connect to the server
  } else {

    if (serialLog) Serial.println("Server failed... ");
    tft.println("Server connection failed... ");
    delay(1000);
    xSemaphoreGive( ( xCommandConnectionSemaphore ) );
    return;

  }

  xSemaphoreGive( ( xCommandConnectionSemaphore ) );

  initRefreshLichessConnectionsTask();

}





void processFenIntoLichessPosition(const char* fen) {

  if (fen != NULL and strlen(fen) >= 15) {

    int j = 0;

    for (int i = 0; i < strlen(fen); i++) {

      if (j > 63) {
        //enough
        break;
      }

      if (fen[i] == '/') {
        //Ignore slash
        continue;
      } else if (fen[i] < 0x3A) {
        //If number add zeros
        for (int z = 0; z < (fen[i] - 0x30 ); z++) {
          lichessBoardPosition[j] = EMPTY;
          j++;

        }
      }
      else {
        //Piece
        switch (fen[i]) {
          case 'P':
            lichessBoardPosition[j] = WPAWN;
            break;
          case 'R':
            lichessBoardPosition[j] = WROOK;
            break;
          case 'N':
            lichessBoardPosition[j] = WKNIGHT;
            break;
          case 'B':
            lichessBoardPosition[j] = WBISHOP;
            break;
          case 'K':
            lichessBoardPosition[j] = WKING;
            break;
          case 'Q':
            lichessBoardPosition[j] = WQUEEN;
            break;
          case 'p':
            lichessBoardPosition[j] = BPAWN;
            break;
          case 'r':
            lichessBoardPosition[j] = BROOK;
            break;
          case 'n':
            lichessBoardPosition[j] = BKNIGHT;
            break;
          case 'b':
            lichessBoardPosition[j] = BBISHOP;
            break;
          case 'q':
            lichessBoardPosition[j] = BQUEEN;
            break;
          case 'k':
            lichessBoardPosition[j] = BKING;
            break;
        }
        j++;
      }
    }
  }
}




void refreshCommandConnection() {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (currentGameID == NULL) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return;
  }

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    if (serialLog) Serial.println("Refreshing connection refreshCommandConnection chat ");

    //keep the request so lichess knows you are there
    wifiClient.print("GET /api/board/game/");
    wifiClient.print(currentGameID);
    wifiClient.println("/chat HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();

    if (processHTTP(&wifiClient) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO refreshCommandConnection chat");
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout refreshCommandConnection");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }
    }

    //DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error || doc.isNull()) {
      if (serialLog) Serial.println("HTTP error refreshCommandConnection chat");
      xSemaphoreGive( xCommandConnectionSemaphore );
      return ;
    }

    lastTimeCommandConnectionUse = millis();

    if (serialLog) Serial.println("Refreshed connection refreshCommandConnection chat");
  } else {
    if (serialLog) Serial.println("Refresh connection chat FAILED");
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

}





void sendLichessResign() {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (currentGameID == NULL) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return;
  }

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    if (serialLog) Serial.println("Refreshing connection sendLichessResign chat ");

    //keep the request so lichess knows you are there
    wifiClient.print("POST /api/board/game/");
    wifiClient.print(currentGameID);
    wifiClient.println("/resign HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();

    if (processHTTP(&wifiClient) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO sendLichessResign");
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout sendLichessResign");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }
    }

    //DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error || doc.isNull()) {
      if (serialLog) Serial.println("HTTP error sendLichessResign");
      xSemaphoreGive( xCommandConnectionSemaphore );
      return ;
    }

    lastTimeCommandConnectionUse = millis();

    if (serialLog) Serial.println("OK sendLichessResign");
  } else {
    if (serialLog) Serial.println("sendLichessResign FAILED");
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

}







void sendLichessDrawOffer(boolean acceptOffer) {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (currentGameID == NULL) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return;
  }

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    //keep the request so lichess knows you are there
    wifiClient.print("POST /api/board/game/");
    wifiClient.print(currentGameID);
    wifiClient.print("/draw/");
    if (acceptOffer) {
      wifiClient.print("yes");
    } else {
      wifiClient.print("no");
    }
    wifiClient.println(" HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();

    if (processHTTP(&wifiClient) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO sendLichessDrawOffer");
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout sendLichessDrawOffer");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }
    }

    //DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error || doc.isNull()) {
      if (serialLog) Serial.println("HTTP error sendLichessDrawOffer");
      xSemaphoreGive( xCommandConnectionSemaphore );
      return ;
    }

    lastTimeCommandConnectionUse = millis();

    if (serialLog) Serial.println("OK sendLichessDrawOffer");
  } else {
    if (serialLog) Serial.println("sendLichessDrawOffer FAILED");
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

}





void sendLichessTakeBack(boolean acceptOffer) {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (currentGameID == NULL) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return;
  }

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    //keep the request so lichess knows you are there
    wifiClient.print("POST /api/board/game/");
    wifiClient.print(currentGameID);
    wifiClient.print("/takeback/");
    if (acceptOffer) {
      wifiClient.print("yes");
    } else {
      wifiClient.print("no");
    }
    wifiClient.println(" HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();

    if (processHTTP(&wifiClient) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO sendLichessTakeBack");
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout sendLichessTakeBack");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }
    }

    //DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error || doc.isNull()) {
      if (serialLog) Serial.println("HTTP error sendLichessTakeBack");
      xSemaphoreGive( xCommandConnectionSemaphore );
      return ;
    }

    lastTimeCommandConnectionUse = millis();

    if (serialLog) Serial.println("OK sendLichessTakeBack");
  } else {
    if (serialLog) Serial.println("sendLichessTakeBack FAILED");
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

}






void refreshStreamConnection() {

  long current = millis();
  if (wifiClientStream.connected() && (lastTimeStreamConnectionUse == 0 || current > lastTimeStreamConnectionUse + 3000)) {

    if (serialLog) Serial.println("Refreshing connection refreshStreamConnection");

    //keep the request so lichess knows you are there
    wifiClientStream.println();
    lastTimeStreamConnectionUse = millis();

    if (serialLog) Serial.println("END Refreshing connection refreshStreamConnection");
  }
}




boolean getRunningGameLichess() {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (!wifiClient.connected()) {
    if (serialLog) Serial.println("Connecting wifiClient getRunningGameLichess...");
    wifiClient.stop();
    if (!wifiClient.connect(server, 443) ) {
      if (serialLog) Serial.println("Error connecting wifiClient getRunningGameLichess...");
      //wifiClientStream.stop();

      //restart wifi
      //if (serialLog) Serial.println("RESTARTING WIFI getRunningGameLichess...");
      //WiFi.disconnect();
      //initializeWifi();

      xSemaphoreGive( xCommandConnectionSemaphore );
      return false;
    }

  }


  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    if (serialLog) Serial.println("Getting game info getRunningGameLichess");

    //keep the request so lichess knows you are there
    wifiClient.println("GET /api/account/playing HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();

    if (processHTTP(&wifiClient) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO getRunningGameLichess");
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return false;
    }

    lastTimeCommandConnectionUse = millis();

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout getRunningGameLichess");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return false;
      }
    }

    //DynamicJsonDocument doc(1024);
    DeserializationError error = deserializeJson(doc, wifiClient);
    if (error || doc.isNull()) {
      if (serialLog) Serial.println("deserializeJson error connectToRunningGameLichess ");

      if (error) {
        // this is due to an error in the HTTP request
        if (serialLog) Serial.print(F("deserializeJson() failed: "));
        if (serialLog) Serial.println(error.f_str());
      }
      wifiClient.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return false;
    }


    // Extract values
    if (doc["nowPlaying"].size() == 0) {
      //No ongoing games

      if (currentGameID != NULL) {
        //Make sure we clear pending draw offers and resign orders
        resetLichessGameActions();
      }

      delete currentGameID;
      currentGameID = NULL;
      if (serialLog) Serial.println("No games found");
      xSemaphoreGive( xCommandConnectionSemaphore );

      return true;
    }

    lastTimeCommandConnectionUse = millis();


    JsonObject currentGameJson;

    boolean foundGame = false;


    if (currentGameID != NULL) {
      for (int i = 0; i < doc["nowPlaying"].size(); i++) {
        const char* gameID = doc["nowPlaying"][i]["gameId"];
        if (strcmp(gameID, currentGameID) == 0) {
          if (serialLog) Serial.println("Found game");
          foundGame = true;
          currentGameJson = doc["nowPlaying"][i];
          delete currentGameID;
          currentGameID = strdup(currentGameJson["gameId"]);

          break;
        }
      }
    }


    if (currentGameID != NULL && !foundGame) {
      delete currentGameID;
      currentGameID = NULL;
      if (serialLog) Serial.println("Game not found ?");
      resetLichessGameActions();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return true;
    }

    if (currentGameID == NULL) {
      
      //Still no game, select first standard non-correspondence game from the list
      int gameIndex = -1;
      for (int i = 0; i < doc["nowPlaying"].size(); i++) {
        JsonObject gameJson = doc["nowPlaying"][i];
        if ( strcmp(gameJson["speed"], "correspondence")!= 0  && strcmp(gameJson["variant"]["key"], "standard") == 0 ) {
          gameIndex = i;
          if (serialLog) Serial.print("Current Game index is: ");
          if (serialLog) Serial.println(i, DEC);
          break;
        }
      }

      if (gameIndex == -1) {
        xSemaphoreGive( xCommandConnectionSemaphore );
        delete currentGameID;
        currentGameID = NULL;
        return true;
      }

      //Make sure lichess TV is not connected
      wifiClientStream.stop();
      currentGameJson = doc["nowPlaying"][0];
      delete currentGameID;
      currentGameID = strdup(currentGameJson["gameId"]);
      resetLichessGameActions();

    }

    if (currentGameID != NULL) {
      // if there is an ongoing game
      if (serialLog) Serial.print("Current Game ID is: ");
      if (serialLog) Serial.println(currentGameID);
      // extract information from the output json
      myTurn =   (currentGameJson["isMyTurn"] == true);
      delete lastMove;
      lastMove = strdup(currentGameJson["lastMove"]);
      delete myColour;
      myColour = strdup(currentGameJson["color"]);
      delete currentFen;
      currentFen = strdup(currentGameJson["fen"]);
      delete opponentName;
      opponentName = strdup(currentGameJson["opponent"]["username"]);

      if (serialLog) Serial.print("Fen: " );
      if (serialLog) Serial.println(currentFen);

      if (serialLog) Serial.print("lastMove: " );
      if (serialLog) Serial.println(lastMove);

      if (serialLog) Serial.print("myTurn: " );
      if (serialLog) Serial.println(myTurn);

      if (serialLog) Serial.print("My color: " );
      if (serialLog) Serial.println(myColour);

      if (serialLog) Serial.print("opponentName: " );
      if (serialLog) Serial.println(opponentName);

      if (strcmp(myColour, "black") == 0) {
        opponentColour = "white";
      } else {
        opponentColour = "black";

      }
    }
    xSemaphoreGive( xCommandConnectionSemaphore );
    return true;
  } else {
    if (serialLog) Serial.println("Connection error getRunningGameLichess");
    xSemaphoreGive( xCommandConnectionSemaphore );
    return false;
  }

  xSemaphoreGive( xCommandConnectionSemaphore );
}





//Need to put in IRAM as it is used by ISR
boolean IRAM_ATTR isLichessGameActive() {
  if (currentGameID != NULL) {
    return true;
  }

  return false;

}


void getLichessTvEvents() {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  //Make sure no game active after semaphore release
  if (isLichessGameActive()) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return;
  }

  if (!wifiClientStream.connected()) {
    if (serialLog) Serial.println("Connecting getLichessTvEvents................................");
    wifiClientStreamInitializedStatus = false;
    if (!wifiClientStream.connect(server, 443) ) {
      if (serialLog) Serial.println("Error connecting getLichessTvEvents...");
      wifiClientStream.stop();

      //restart wifi
      //if (serialLog) Serial.println("RESTARTING WIFI getLichessTvEvents...");
      //WiFi.disconnect();
      //initializeWifi();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }

  }

  if (!wifiClientStreamInitializedStatus ) {

    if (serialLog) Serial.println("GET TV FEED getLichessTvEvents");

    //keep the request so lichess knows you are there
    wifiClientStream.print("GET /api/tv/feed");
    wifiClientStream.println(" HTTP/1.1");
    wifiClientStream.println("Host: lichess.org");
    wifiClientStream.print("Authorization: Bearer ");
    wifiClientStream.println(token);
    wifiClientStream.println();

    if (processHTTP(&wifiClientStream) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO getLichessTvEvents");
      wifiClientStream.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }
  }

  //if (serialLog) Serial.println("LI1b");

  //if (serialLog) Serial.println("GET DATA getLichessTvEvents");

  wifiClientStreamInitializedStatus = true;

  if (wifiClientStream.connected() && wifiClientStream.available()) {

    char* line = getStatusLineFromClient();

    //if (serialLog) Serial.println("LI1c");

    //if (serialLog) Serial.println("Returned status line: ");
    //if (serialLog) Serial.println(line);

    if (strlen(line) > 4) {

      //if (serialLog) Serial.println("Linea recibida: ");
      //if (serialLog) Serial.println(line);


      DeserializationError error = deserializeJson(doc, line);
      if (error) {
        if (serialLog) Serial.println("deserializeJson error getIncomingGameEventsLichessTV ");
        // this is due to an error in the HTTP request
        if (serialLog) Serial.print(F("deserializeJson() failed: "));
        if (serialLog) Serial.println(error.f_str());
        delete line;
        xSemaphoreGive( xCommandConnectionSemaphore );
        return;
      }

      const char* type = doc["t"];

      //if (serialLog) Serial.print("Event Type: ");
      //if (serialLog) Serial.println(type);

      const char* fen = NULL;
      const char* lastmove = NULL;
      const char* orientation = NULL;

      if (0 == strcmp(type , "featured")) {

        lichessStreamTvActive = true;

        fen = doc["fen"];
        orientation = doc["d"]["orientation"];
        //if (serialLog) Serial.print("Orientation: ");
        //if (serialLog) Serial.print(orientation);
        delete myColour;
        myColour = strdup(orientation);
        if (orientation != NULL) {
          if (strcmp(orientation, "black") == 0) {
            boardInverted = true;
            if (serialLog) Serial.println("TV reversed");
          } else {
            boardInverted = false;
            if (serialLog) Serial.println("TV not reversed");
          }

        }

        if (strcmp(doc["d"]["players"][0]["color"], "white") == 0) {

          lichessWhiteName =   doc["d"]["players"][0]["user"]["name"];
          lichessBlackName =   doc["d"]["players"][1]["user"]["name"];

          if (!doc["d"]["players"][0]["user"]["title"].isNull()) {
            lichessWhiteTitle =  doc["d"]["players"][0]["user"]["title"];
          } else {
            lichessWhiteTitle = "";
          }

          if (!doc["d"]["players"][1]["user"]["title"].isNull()) {
            lichessBlackTitle =  doc["d"]["players"][1]["user"]["title"];
          } else {
            lichessBlackTitle = "";
          }

          lichessWhiteRating = doc["d"]["players"][0]["rating"];
          lichessBlackRating = doc["d"]["players"][1]["rating"];

        } else {

          lichessWhiteName =   doc["d"]["players"][1]["user"]["name"];
          lichessBlackName =   doc["d"]["players"][0]["user"]["name"];

          if (!doc["d"]["players"][1]["user"]["title"].isNull()) {
            lichessWhiteTitle =  doc["d"]["players"][1]["user"]["title"];
          } else {
            lichessWhiteTitle = "";
          }

          if (!doc["d"]["players"][0]["user"]["title"].isNull()) {
            lichessBlackTitle =  doc["d"]["players"][0]["user"]["title"];
          } else {
            lichessBlackTitle = "";
          }

          lichessWhiteRating = doc["d"]["players"][1]["rating"];
          lichessBlackRating = doc["d"]["players"][0]["rating"];

        }

        //if (serialLog) Serial.println("LI1d");

        paintLichessPlayerClear();

        if (!boardInverted) {

          paintLichessPlayerTopSprite(lichessBlackTitle, lichessBlackName, lichessBlackRating);
          paintLichessPlayerBottomSprite(lichessWhiteTitle, lichessWhiteName, lichessWhiteRating);
        } else {
          paintLichessPlayerTopSprite(lichessWhiteTitle, lichessWhiteName, lichessWhiteRating);
          paintLichessPlayerBottomSprite(lichessBlackTitle, lichessBlackName, lichessBlackRating);

        }

        lastmove = NULL;
        //if (serialLog) Serial.print("Fen: ");
        //if (serialLog) Serial.println(fen);
      } else  if (0 == strcmp(type , "fen")) {
        lastmove = doc["d"]["lm"];
        fen = doc["d"]["fen"];
        lichessColorToMove = fen[strlen(fen) - 1];

        xSemaphoreTake( xLichessClockSemaphore, ( TickType_t ) lichessSemaphoreTimeout);

        lichessWhiteTime = doc["d"]["wc"];
        lichessBlackTime = doc["d"]["bc"];

        lichessWhiteTime = lichessWhiteTime * 1000;
        lichessBlackTime = lichessBlackTime * 1000;

        xSemaphoreGive( xLichessClockSemaphore );

        if ((lichessColorToMove == 'b' && !boardInverted) || (lichessColorToMove == 'w' && boardInverted) ) {
          myTurn = false;
        } else {
          myTurn = true;
        }

        if (!boardInverted) {
          paintLichessTimeTop(lichessBlackTime, !myTurn);
          paintLichessTimeBottom(lichessWhiteTime, myTurn);
        } else {
          paintLichessTimeTop(lichessWhiteTime, !myTurn);
          paintLichessTimeBottom(lichessBlackTime, myTurn);
        }

        //if (serialLog) Serial.print("Last move: ");
        //if (serialLog) Serial.println(lastmove);
      }

      //Process LASTMOVE into screen marking
      for (int i = 0; i < 64; i++) {
        lichessBoardPositionMarked[i] = 0;
      }

      if (lastmove != NULL) {
        //extract the 2 marked squares
        //if (serialLog) Serial.print("Loading LASTMOVE: ");
        //if (serialLog) Serial.println(lastmove);

        if (strlen(lastmove) >= 4 && lastmove[0] != 0x20 && lastmove[1] != 0x20 && lastmove[2] != 0x20 && lastmove[3] != 0x20) {
          byte firstSquare = (8 - (lastmove[1] - 0x30)) * 8 + (lastmove[0] - 0x61);
          byte secondSquare = (8 - (lastmove[3] - 0x30)) * 8 + (lastmove[2] - 0x61);

          lichessBoardPositionMarked[firstSquare] = 1;
          lichessBoardPositionMarked[secondSquare] = 1;
        }
      }

      //Process FEN into screen position

      processFenIntoLichessPosition(fen);

    }
    delete line;
    //if (serialLog) Serial.println("LI1e");
  }

  refreshStreamConnection();

  xSemaphoreGive( xCommandConnectionSemaphore );

}






void getIncomingGameEventsLichess() {

  lichessStreamTvActive = false;

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (!wifiClientStream.connected()) {
    if (serialLog) Serial.println("Connecting getIncomingGameEventsLichess...");
    wifiClientStreamInitializedStatus = false;
    if (!wifiClientStream.connect(server, 443) ) {
      if (serialLog) Serial.println("Error connecting getIncomingGameEventsLichess...");
      wifiClientStream.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }
  }

  if (!wifiClientStreamInitializedStatus ) {
    //keep the request so lichess knows you are there
    wifiClientStream.print("GET /api/board/game/stream/");
    wifiClientStream.print(currentGameID);
    wifiClientStream.println(" HTTP/1.1");
    wifiClientStream.println("Host: lichess.org");
    wifiClientStream.print("Authorization: Bearer ");
    wifiClientStream.println(token);
    wifiClientStream.println();

    if (processHTTP(&wifiClientStream) != 200) {
      if (serialLog) Serial.println("processHTTP FALLO getIncomingGameEventsLichess");
      wifiClientStream.stop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return;
    }
  }

  wifiClientStreamInitializedStatus = true;

  if (wifiClientStream.connected() && wifiClientStream.available()) {

    char* line = getStatusLineFromClient();

    //if (serialLog) Serial.println("Returned status line: ");
    //if (serialLog) Serial.println(line);

    if (strlen(line) > 4) {

      //We only get the clock time from this event

      if (serialLog) Serial.println("Linea recibida /api/board/game/stream/: ");
      if (serialLog) Serial.println(line);

      //DynamicJsonDocument doc(1024);
      DeserializationError error = deserializeJson(doc, line);
      if (error) {
        if (serialLog) Serial.println("deserializeJson error getIncomingGameEventsLichess ");
        if (serialLog) Serial.print(F("deserializeJson() failed: "));
        if (serialLog) Serial.println(error.f_str());
        xSemaphoreGive( xCommandConnectionSemaphore );
        delete line;
        return;
      }

      const char* type = doc["type"];

      if (serialLog) Serial.print("Event getIncomingGameEventsLichess Type: ");
      if (serialLog) Serial.println(type);

      const char* fen = NULL;
      const char* moves = NULL;
      char* lastmove = NULL;
      const char* orientation = NULL;


      if (0 == strcmp(type , "gameFull")) {

        if (serialLog) Serial.println("Processing gameFull ");

        moves = doc["state"]["moves"];

        char* moves2 = strdup(moves);
        char* token = strtok(moves2, " ");
        while ( token != NULL ) {
          if (token != NULL ) {
            delete lastmove;
            lastmove = strdup(token);
          }
          token = strtok(NULL, " ");
        }

        delete moves2;


        //XXXX
        xSemaphoreTake( xLichessClockSemaphore, ( TickType_t ) lichessSemaphoreTimeout);

        lichessWhiteTime = doc["state"]["wtime"];
        lichessBlackTime = doc["state"]["btime"];

        xSemaphoreGive( xLichessClockSemaphore );


        if (serialLog) Serial.print("White time: ");
        if (serialLog) Serial.println(lichessWhiteTime, DEC);
        if (serialLog) Serial.print("Black time: ");
        if (serialLog) Serial.println(lichessBlackTime, DEC);

      }


      else if (0 == strcmp(type , "gameState")) {

        if (serialLog) Serial.println("Processing Gamestate ");


        //Detect if game has finished
        const char* gameStatus = doc["status"];
        const char* winner = doc["winner"];

        if (gameStatus != NULL) {
          if (serialLog) Serial.print("Gamestatus: ");
          if (serialLog) Serial.println(gameStatus);
        }

        if (! ((0 == strcmp(gameStatus , "created")) || (0 == strcmp(gameStatus , "started"))  ) ) {
          //Game finished
          resetLichessGameActions();
          delete currentGameID;
          currentGameID = NULL;
          if ((0 == strcmp(gameStatus , "stalemate")) || (0 == strcmp(gameStatus , "draw"))   )  {
            paintLichessGameEndedDraw();
          } else {
            if (winner != NULL) {
              if (0 == strcmp(winner , "white")) {
                if (!boardInverted) {
                  paintLichessGameWonBottom();
                } else {
                  paintLichessGameWonTop();
                }
              }

              if (0 == strcmp(winner , "black")) {
                if (!boardInverted) {
                  paintLichessGameWonTop();
                } else {
                  paintLichessGameWonBottom();
                }
              }
            }
          }
        }

        //Detect if draw offer or takeback proposal received

        boolean wDrawOffer = doc["wdraw"];
        boolean wTakebackProposal = doc["wtakeback"];

        boolean bDrawOffer = doc["bdraw"];
        boolean bTakebackProposal = doc["btakeback"];

        if  (
          (strcmp(myColour, "white") == 0 && bDrawOffer )
          ||
          (strcmp(myColour, "black") == 0 && wDrawOffer)
        ) {
          if (serialLog) Serial.println("OPPONENT DRAW OFFER!!-------------");
          pendingLichessOpponentRequest = OPPONENT_DRAW_OFFER;
          askForLichessConfirmationOpponentDrawOffer();
        }


        if (
          (strcmp(myColour, "black") == 0 && wTakebackProposal)
          ||
          (strcmp(myColour, "white") == 0 && bTakebackProposal)
        ) {
          if (serialLog) Serial.println("OPPONENT TAKEBACK PROPOSAL!!-------------");
          pendingLichessOpponentRequest = OPPONENT_TAKEBACK_PROPOSAL;
          askForLichessConfirmationOpponentTakebackProposal();

        }




        moves = doc["moves"];

        char* moves2 = strdup(moves);
        char* token = strtok(moves2, " ");
        while ( token != NULL ) {
          if (token != NULL ) {
            delete lastmove;
            lastmove = strdup(token);
          }
          token = strtok(NULL, " ");
        }

        delete moves2;

        xSemaphoreTake( xLichessClockSemaphore, ( TickType_t ) lichessSemaphoreTimeout);

        lichessWhiteTime = doc["wtime"];
        lichessBlackTime = doc["btime"];

        xSemaphoreGive( xLichessClockSemaphore );


        if (serialLog) Serial.print("White time: ");
        if (serialLog) Serial.println(lichessWhiteTime, DEC);
        if (serialLog) Serial.print("Black time: ");
        if (serialLog) Serial.println(lichessBlackTime, DEC);

        if (!myTurn) {
          beep();
          delay(100);
          beep();
        }




      }  else {
        //Chat or Unknown
        if (serialLog) Serial.println("Unsupported command:");
        xSemaphoreGive( xCommandConnectionSemaphore );
        delete line;
        return;
      }


      if (serialLog) Serial.println("Get rest of Gamestate data ");
      //Now get rest of data from /api/account/playing

      xSemaphoreGive( xCommandConnectionSemaphore );

      while (!getRunningGameLichess()) {
        if (serialLog) Serial.println("Retrying getRunningGameLichess");
      }

      xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );


      if (currentGameID == NULL) {
        if (serialLog) Serial.println("JSON error getIncomingGameEventsLichess ");
        xSemaphoreGive( xCommandConnectionSemaphore );
        delete line;
        return;
        //Game aborted
        if (serialLog) Serial.println("Game aborted");
        xSemaphoreGive( xCommandConnectionSemaphore );
        delete line;
        return;
      }

      paintLichessPlayerClear();

      paintLichessPlayerTopSprite("", opponentName, NULL);
      paintLichessPlayerBottomSprite("", username, NULL);


      if (strcmp(lichessAutoInvertEnabled, "TRUE")==0) {
        if (strcmp(myColour, "white") == 0) {
          boardInverted = false;
          if (serialLog) Serial.print("Board NOT reversed: ");
          if (serialLog) Serial.println(myColour);
        } else {
          if (serialLog) Serial.print("Board REVERSED: ");
          if (serialLog) Serial.println(myColour);
          boardInverted = true;
        }
      }

      //Process FEN into screen position
      processFenIntoLichessPosition(currentFen);

      //      if (!boardInverted) {
      //        paintLichessTimeTop(lichessBlackTime, !myTurn);
      //        paintLichessTimeBottom(lichessWhiteTime, myTurn);
      //      } else {
      //        paintLichessTimeTop(lichessWhiteTime, !myTurn);
      //        paintLichessTimeBottom(lichessBlackTime, myTurn);
      //      }




      //Process LASTMOVE into screen marking
      for (int i = 0; i < 64; i++) {
        lichessBoardPositionMarked[i] = 0;
      }

      if (serialLog) Serial.println("Processing lastmove");

      if (lastmove != NULL) {
        //extract the 2 marked squares
        //if (serialLog) Serial.print("Loading LASTMOVE: ");
        //if (serialLog) Serial.println(lastmove);

        if (strlen(lastmove) >= 4 && lastmove[0] != 0x20 && lastmove[1] != 0x20 && lastmove[2] != 0x20 && lastmove[3] != 0x20) {
          byte firstSquare = (8 - (lastmove[1] - 0x30)) * 8 + (lastmove[0] - 0x61);
          byte secondSquare = (8 - (lastmove[3] - 0x30)) * 8 + (lastmove[2] - 0x61);

          lichessBoardPositionMarked[firstSquare] = 1;
          lichessBoardPositionMarked[secondSquare] = 1;
        }

        delete lastmove;
      }
    }
    delete line;
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

  //if (serialLog) Serial.println("Exiting getIncomingGameEventsLichess");
}








void fixBoardPositionLichessData() {
  //In lichess mode there is no point in tracking all the pieces, the server is only interested in coordinated and moves
  //So we fix board position based on lichess FEN



  //First case: if all the scanned cells match we assume everything is ok
  boolean allMatch = true;
  for (int i = 0; i < 64; i++) {

    int index = i;
//    if (boardInverted) {
//      index = 63 - i;
//    }

    if (scannedpos[index] == false && lichessBoardPosition[i] != 0 || scannedpos[index] == true && lichessBoardPosition[i] == 0) {
      allMatch = false;
      break;
    }
  }

  if (allMatch) {
    for (int i = 0; i < 64; i++) {
      int index = i;
      if (boardInverted) {
        index = 63 - i;
      }
      currentPos[index] = lichessBoardPosition[i];
    }
  }



  //Second case: not all the scanned cells match we fix selectively

  if (!allMatch) {
    for (int i = 0; i < 64; i++) {

      int index = i;
      if (boardInverted) {
        index = 63 - i;
      }

      if (
        (scannedpos[index] == false  && lichessBoardPosition[i] == 0 )  //empty squares both sides
        ||
        (scannedpos[index] == true && lichessBoardPosition[i] != 0 && lichessBoardPositionMarked[i] == 0) //occupied squares both sides not in last move
        ||
        (scannedpos[index] == true && lichessBoardPosition[i] == 0) //occupied squares in the board that should be empty
      ) {
        currentPos[index] = lichessBoardPosition[i];
      }
    }

  }





}





void setLichessBoardLeds() {

  //Light up all the leds where the current board position is different from the Lichess board position
  for (int i = 0; i < 64; i++) {

    int index = i;
    if (boardInverted) {
      index = 63 - i;
    }

    byte valueToSet = 0;
    if (currentPos[index] != lichessBoardPosition[i] || lichessBoardPosition[i] != 0 && scannedpos[index] == false || lichessBoardPosition[i] == 0 && scannedpos[index] == true) {
      valueToSet = 4;
    }


    for (int j = 0; j < 8; j++) {
      ledStatusBuffer[63 - index + (64 * j)] = valueToSet;
    }
  }



}



boolean sendOutgoingMovementLichess(byte originSquare, byte destinationSquare) {

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  char movement[6];

  char promotionPiece = 0;

  //Detect white promotion
  if (((destinationSquare / 8) == 0) && (lichessBoardPosition[originSquare] == 1)) {
    promotionPiece = 'Q';

    //Alternative piece TBD
  }

  //Detect black promotion
  if (((destinationSquare / 8) == 7) && (lichessBoardPosition[originSquare] == 7)) {
    promotionPiece = 'q';

    //Alternative piece TBD
  }

  movement[0] = char(originSquare % 8 + 0x61);
  movement[1] = char(8 - (originSquare / 8) + 0x30);
  movement[2] = char(destinationSquare % 8 + 0x61);
  movement[3] = char(8 - (destinationSquare / 8) + 0x30);
  movement[4] = promotionPiece;
  movement[5] = 0;

  if (strcmp(lastMove, movement) == 0) {
    xSemaphoreGive( xCommandConnectionSemaphore );
    return true;
  }

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    if (serialLog) Serial.print("Sending movement on sendOutgoingMovementLichess: ");

    if (serialLog) Serial.println(movement);

    //keep the request so lichess knows you are there
    wifiClient.print("POST /api/board/game/");
    wifiClient.print(currentGameID);
    wifiClient.print("/move/");
    wifiClient.print(movement);
    wifiClient.println(" HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.println();



    int returnCode = processHTTP(&wifiClient);
    if (returnCode != 200 && returnCode != 400) {
      if (serialLog) Serial.print("processHTTP FALLO sendOutgoingMovementLichess: ");
      if (serialLog) Serial.println(returnCode, DEC);
      xSemaphoreGive( xCommandConnectionSemaphore );
      return false;
    }

    int timeout = millis() + 2000;
    while (wifiClient.available() == 0) {
      if (timeout - millis() < 0) {
        if (serialLog) Serial.println("Client Timeout sendOutgoingMovementLichess");
        wifiClient.stop();
        xSemaphoreGive( xCommandConnectionSemaphore );
        return false;
      }
    }

    char* line = getLineFromClient(&wifiClient);

    if (serialLog) Serial.print("Leida linea: ");
    if (serialLog) Serial.println(line);

    //StaticJsonDocument<48> doc;
    DeserializationError error = deserializeJson(doc, line);

    delete line;

    lastTimeCommandConnectionUse = millis();

    if (error) {
      if (serialLog) Serial.println("deserializeJson error sendOutgoingMovementLichess ");
      if (serialLog) Serial.print(F("deserializeJson() failed: "));
      if (serialLog) Serial.println(error.f_str());
      xSemaphoreGive( xCommandConnectionSemaphore );
      return false;
    }

    //determine whether the move was successful
    moveSuccess = doc["ok"];
    if (moveSuccess == true) {
      xSemaphoreGive( xCommandConnectionSemaphore );
      return true;

    }
    else {
      boop();
      delay(100);
      boop();
      delay(100);
      boop();
      xSemaphoreGive( xCommandConnectionSemaphore );
      return true;
    }
  }
  xSemaphoreGive( xCommandConnectionSemaphore );
  return false;
}



boolean isMyTurnLichess() {
  return myTurn;
}



void keepAliveLichess( void * pvParameters ) {

  for (;;) {
    long current = millis();

    //if (serialLog) Serial.println("Tick...");
    //if (serialLog) Serial.println(lastTimeCommandConnectionUse, DEC);
    //if (serialLog) Serial.println(wifiClientStreamInitializedStatus, DEC);

    long delayToPing = 3000l;

    if (!isLichessGameActive()) {
      delayToPing = 5000l;
    }


    if (wifiClientStreamInitializedStatus && lastTimeCommandConnectionUse != 0 && current > (lastTimeCommandConnectionUse + delayToPing))  {
      //if ( xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) 0 ) == pdTRUE ) {

      if (serialLog) Serial.println("Refresh in");


      if (isLichessGameActive()) {
        if (serialLog) Serial.println("Refresh via chat");
        refreshCommandConnection();
      }

      else {
        if (serialLog) Serial.println("Refresh via list games");
        getRunningGameLichess();
      }

      if (serialLog) Serial.println("Refresh out");

      //xSemaphoreGive( xCommandConnectionSemaphore );
      //}
    }

    //    if (isLichessConnected && !isLichessGameActive() && wifiClientStreamInitializedStatus) {
    //      refreshStreamConnection();
    //    }

    delay(500);
  }
}







void initRefreshLichessConnectionsTask() {

  if (refreshLichessConnectionsTaskEnabled) {
    return;
  }

  refreshLichessConnectionsTaskEnabled = true;

  //if more than 3 seconds since last request on command connection, send dummy command to keep it alive

  TaskHandle_t Task1;

  xTaskCreatePinnedToCore(
    keepAliveLichess, /* Task function. */
    "LichessKeepAlive",   /* name of task. */
    10000,     /* Stack size of task */
    NULL,      /* parameter of the task */
    1,         /* priority of the task */
    &Task1,    /* Task handle to keep track of created task */
    1);        /* Core number */
}
/* if (serialLog) Serial.println("");
  if (serialLog) Serial.println("Scanned");

  for (int i = 0; i < 8; i++) {
   for (int j = 0; j < 8; j++) {
     if (serialLog) Serial.print(scannedpos[(8 * i) + j ],  HEX);
   }
   if (serialLog) Serial.println("");
  }
  if (serialLog) Serial.println("");
  if (serialLog) Serial.println("Current");

  for (int i = 0; i < 8; i++) {
   for (int j = 0; j < 8; j++) {
     if (serialLog) Serial.print(currentPos[(8 * i) + j ],  HEX);
   }
   if (serialLog) Serial.println("");
  }

  if (serialLog) Serial.println("");
  if (serialLog) Serial.println("Lichess");

  for (int i = 0; i < 8; i++) {
   for (int j = 0; j < 8; j++) {
     if (serialLog) Serial.print(lichessBoardPosition[(8 * i) + j ],  HEX);
   }
   if (serialLog) Serial.println("");
  }

  }*/



void refreshLichessClocks() {

  //Only refresh if game active
  if (lichessStreamTvActive || isLichessGameActive()) {

    if (!boardInverted) {
      paintLichessTimeTop(lichessBlackTime, !myTurn);
      paintLichessTimeBottom(lichessWhiteTime, myTurn);
    } else {
      paintLichessTimeTop(lichessWhiteTime, !myTurn);
      paintLichessTimeBottom(lichessBlackTime, myTurn);
    }
  }
  delay(10);
}





void launchLichessSeek(const char* lichessTime, const char* lichessIncrement, const char* lichessRated, const char* lichessColor) {

  showLichessSeekScreen();

  xSemaphoreTake( xCommandConnectionSemaphore, ( TickType_t ) lichessSemaphoreTimeout );

  if (wifiClient.connected() || wifiClient.connect(server, 443)) {

    if (serialLog) Serial.print("launchLichessSeek: ");

    char body[200];
    sprintf(body, "time=%s&increment=%s&color=%s&rated=%s&variant=standard", lichessTime, lichessIncrement, lichessColor, lichessRated );

    if (serialLog) Serial.println(body);
    if (serialLog) Serial.print("Len: ");
    if (serialLog) Serial.println(strlen(body), DEC);

    //keep the request so lichess knows you are there
    wifiClient.print("POST /api/board/seek");
    wifiClient.println(" HTTP/1.1");
    wifiClient.println("Host: lichess.org");
    wifiClient.print("Authorization: Bearer ");
    wifiClient.println(token);
    wifiClient.print("Content-Length: ");
    wifiClient.println(strlen(body), DEC);
    wifiClient.println("Content-Type: application/x-www-form-urlencoded");
    wifiClient.println();
    wifiClient.println(body);

    int returnCode = processHTTP(&wifiClient);
    if (returnCode != 200) {
      if (serialLog) Serial.print("processHTTP FALLO launchLichessSeek: ");
      if (serialLog) Serial.println(returnCode, DEC);
      xSemaphoreGive( xCommandConnectionSemaphore );
      cancelLichessSeekScreen();
      return;
    }

    if (serialLog) Serial.println("launchLichessSeek returned HTTP 200 !!");



    //Seek launched, active until connection is closed


    while (wifiClient.connected()) {

      if (serialLog) Serial.println("Still searching....");

      if (wifiClient.available()) {

        char* line = new char[100];
        memset(line, 0, 100);
        wifiClient.readBytesUntil('\n', line, 100);

        if (serialLog) Serial.print("Leido linea: ");
        if (serialLog) Serial.println(line);
        if (serialLog) Serial.print("size:");
        if (serialLog) Serial.println(strlen(line), DEC);
        lastTimeCommandConnectionUse = millis();
        if (line[0] == '0' ) {
          if (serialLog) Serial.println("Maybe a match!");
          delete line;
          break;
        }

        delete line;
      }


    }

    cancelLichessSeekScreen();


    if (serialLog) Serial.println("Search complete");

    wifiClient.stop();
    wifiClient.connect(server, 443);

    //Check if game was actually launched
    if (serialLog) Serial.println("launchLichessSeek checking if game launched with getRunningGameLichess");
    xSemaphoreGive( xCommandConnectionSemaphore );
    getRunningGameLichess();
    return;
  }

  xSemaphoreGive( xCommandConnectionSemaphore );

  cancelLichessSeekScreen();



}




void lichessButtonPush(byte button) {

  if (isLichessGameActive()) {

    if (pendingLichessOpponentRequest) {
      if (button == BUTTON_BACK) {

        if (pendingLichessOpponentRequest == OPPONENT_DRAW_OFFER) {
          if (serialLog) Serial.println("ACCEPT DRAW OFFER--------------");
          sendLichessDrawOffer(true);
        }

        if (pendingLichessOpponentRequest == OPPONENT_TAKEBACK_PROPOSAL) {
          if (serialLog) Serial.println("ACCEPT TAKEBACK OFFER--------------");
          sendLichessTakeBack(true);
          //TBD
        }

      } else {

        if (pendingLichessOpponentRequest == OPPONENT_DRAW_OFFER) {
          if (serialLog) Serial.println("REJECT DRAW OFFER--------------");
          sendLichessDrawOffer(false);
        }

        if (pendingLichessOpponentRequest == OPPONENT_TAKEBACK_PROPOSAL) {
          if (serialLog) Serial.println("REJECT TAKEBACK OFFER--------------");
          sendLichessTakeBack(false);
        }

      }

      pendingLichessOpponentRequest = 0;
      cancelLichessConfirmation();
      return;
    }

    if (!pendingLichessActionConfirmation) {

      switch (button) {
        case BUTTON_PLUS:
          askForLichessConfirmationTakeBackOffer();
          break;
        case BUTTON_NEXT:
          //Enable overlay to ask for confirmation
          askForLichessConfirmationDrawOffer();
          break;

        case BUTTON_LEVER:
          askForLichessConfirmationResign();
          break;
      }

      pendingLichessActionConfirmation = button;
      return;

    } else {
      if (pendingLichessActionConfirmation != button) {
        //Abort
        pendingLichessActionConfirmation = 0;
        cancelLichessConfirmation();
        return;
      }


    }


    //Second push, same button that first push, launch action over command connection
    pendingLichessActionConfirmation = 0;
    cancelLichessConfirmation();
    if (button == BUTTON_PLUS) {
      if (serialLog) Serial.println("RECORDING TAKEBACK OFFER");
      sendLichessTakeBack(true);
      return;
    }

    if (button == BUTTON_NEXT) {
      if (serialLog) Serial.println("RECORDING DRAW OFFER");
      sendLichessDrawOffer(true);
      return;
    }

    if (button == BUTTON_LEVER) {
      if (serialLog) Serial.println("RESIGNING");
      sendLichessResign();
      return;
    }


    return;

  }


  char* lichessTime;
  char* lichessIncrement;
  char* lichessRated;
  char* lichessColor;

  switch (button) {
    case BUTTON_BACK:
      lichessTime = lichesst1;
      lichessIncrement = lichessi1;
      lichessRated = lichessr1;
      lichessColor = lichessc1;
      break;

    case BUTTON_MINUS:
      lichessTime = lichesst2;
      lichessIncrement = lichessi2;
      lichessRated = lichessr2;
      lichessColor = lichessc2;
      break;

    case BUTTON_PLAY:
      lichessTime = lichesst3;
      lichessIncrement = lichessi3;
      lichessRated = lichessr3;
      lichessColor = lichessc3;
      break;

    case BUTTON_PLUS:
      lichessTime = lichesst4;
      lichessIncrement = lichessi4;
      lichessRated = lichessr4;
      lichessColor = lichessc4;
      break;

    case BUTTON_NEXT:
      lichessTime = lichesst5;
      lichessIncrement = lichessi5;
      lichessRated = lichessr5;
      lichessColor = lichessc5;
      break;

    case BUTTON_LEVER:
      lichessTime = lichesst6;
      lichessIncrement = lichessi6;
      lichessRated = lichessr6;
      lichessColor = lichessc6;
      break;

    default:
      return;
      break;

  }

  if (lichessTime == "" || lichessIncrement == "" || lichessRated == "" || lichessColor == "") {
    return;
  }

  if (!secondLichessButtonPush) {

    //Enable overlay to ask for confirmation
    askForLichessConfirmation(lichessTime, lichessIncrement, lichessRated, lichessColor);
    secondLichessButtonPush = button;

  } else {
    if (secondLichessButtonPush != button) {
      //Abort
      secondLichessButtonPush = 0;
      cancelLichessConfirmation();
      return;
    }


    //Second push, same button that first push, launch seek over command connection
    secondLichessButtonPush = 0;
    cancelLichessConfirmation();
    if (serialLog) Serial.println("LAUNCHING SEEK");
    launchLichessSeek(lichessTime, lichessIncrement, lichessRated, lichessColor);

  }


}


void resetLichessGameActions() {
  secondLichessButtonPush = 0;
  pendingLichessActionConfirmation = 0;
  pendingLichessOpponentRequest = 0;
  cancelLichessConfirmation();
}
