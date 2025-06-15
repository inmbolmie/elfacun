/*
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license

    This program is distributed WITHOUT ANY WARRANTY, even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "ble_board_b.h"


BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;
BLEService *pService;

boolean bleDeviceRequestedUpdateFile = false;

boolean isDiablilloConnected = false;



void startAdvertisement() {

  pServer->getAdvertising()->setAppearance(GENERIC_HID);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setShortName(modeBBtAdvertisedName);
  oAdvertisementData.setName(modeBBtAdvertisedName);
  oAdvertisementData.setAppearance(GENERIC_HID);
  oAdvertisementData.setPartialServices(BLEUUID::fromString("31f4"));

  pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);

  pServer->getAdvertising()->setScanResponse(true);

  pServer->getAdvertising()->start();

}

void stopAdvertisement() {

  pServer->getAdvertising()->stop();

}




void ackMB_UPDATE_INFO(uint32_t size, char* crc)
{

  byte responseData1[20];
  byte responseData2[4];
  byte responseData3[11];


  //ack
  for (int i = 0; i < 20; i++) {
    responseData1[i] = 0x33;
  }


  //size
  responseData2[0] = (uint8_t)(size >> 24);
  responseData2[1] = (uint8_t)(size >> 16);
  responseData2[2] = (uint8_t)(size >> 8);
  responseData2[3] = (uint8_t)(size >> 0);


  //crc
  if (size > 0) {
    memcpy(responseData3, crc, 11);
  }

  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    sendDataTypeBBoard(responseData1, 20);
    delay(100);
    sendDataTypeBBoard(responseData2, 4);
    delay(100);
    sendDataTypeBBoard(responseData3, 11);

    xSemaphoreGive( xSendDataBLESemaphore );
  }

}





//Callback for BLE data reception
class MyCallbacks: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValueRaw = pCharacteristic->getValue();

      //TBM SPLIT MULTIPLE COMMANDS INSIDE rxValue

      if (rxValueRaw.length() > 0) {

        int chunk = 1;

        if (serialDebug) {

          if (serialDebug) Serial.println(" ");
          if (serialDebug)  Serial.print("RECEIVED DATA LEN: ");
          if (serialDebug)  Serial.print(rxValueRaw.length(), DEC);
          if (serialDebug)  Serial.print(" DATA: ");
          for (int i = 0; i < rxValueRaw.length(); i++) {
            if (serialDebug)  Serial.print(rxValueRaw[i], HEX);
            if (serialDebug)  Serial.print(" ");
          }

          if (serialDebug) Serial.println(" ");

        }


        int bytesToProcess = rxValueRaw.length();
        int indexToProcess = 0;

        while (bytesToProcess > 0) {


          int bytesForCommand = 0;

          //first case: we are in the middle of a LED command
          if (!ledbuffer.empty()) {

            bytesForCommand = min(bytesToProcess, (167 - (int)ledbuffer.size()));

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_L_LED) {

            bytesForCommand = min(bytesToProcess, 167);

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_V_VERSION) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_X_OFFLED) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_S_STATUS) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_R_READ) {

            bytesForCommand = 5;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_W_WRITE) {

            bytesForCommand = 7;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_ADV_WRITE) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_XLED_WRITE) {

            bytesForCommand = 19;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_UPDATE_INFO) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  MB_UPDATE_READ) {

            bytesForCommand = 3;

          } else {
            //error
            bytesToProcess = 0;
            if (serialLog) Serial.println("ERROR, BYTES REMAINING WHILE PROCESSING COMMANDS");
            ledbuffer.clear();
            return;
          }

          if (bytesForCommand > bytesToProcess) {
            if (serialLog) Serial.println("ERROR, INCOMPLETE BLE COMMAND PROCESSING COMMANDS");
            ledbuffer.clear();
            return;
          }

          std::string rxValue = rxValueRaw.substr(indexToProcess, bytesForCommand);
          bytesToProcess -= bytesForCommand;
          indexToProcess += bytesForCommand;


          if (extraBLElog && rxValue.length() > 0) {

            if (extraBLElog)  Serial.print("SPLIT DATA LEN: ");
            if (extraBLElog)  Serial.print(rxValue.length(), DEC);
            if (extraBLElog)  Serial.print(" DATA CHUNK NR: ");
            if (extraBLElog)  Serial.print(chunk++, DEC);
            if (extraBLElog)  Serial.print(" : ");
            for (int i = 0; i < rxValue.length() && i < 12 ; i++) {
              if (extraBLElog)  Serial.print(rxValue[i], HEX);
              if (extraBLElog)  Serial.print(" ");
            }

            if (extraBLElog) Serial.println(" ");

          }


          //Begin alternatives
          if (!ledbuffer.empty() && (ledbuffer.size() + rxValue.length()) > 167) {
             ledbuffer.clear();
             if (serialLog)  Serial.println("ERROR RECEIVING LED INFO MESSAGE TOO BIG");
          }


          
          else if (!ledbuffer.empty()) {
            for (int i = 0; i < rxValue.length(); i++) {
              ledbuffer.push_back(rxValue[i]);
            }

            if (ledbuffer.size() == 167) {
              ackMB_L_LED();
              processLedBuffer();
              ledbuffer.clear();

            }
            if (ledbuffer.size() > 167) {
              //Error
              ledbuffer.clear();
              if (serialLog)  Serial.println("ERROR RECEIVING LED INFO BUFFER TOO BIG");
            }

          }

          //response
          else if ((rxValue[0] & 0x7F) ==  MB_L_LED) {

            ledbuffer.clear();
            for (int i = 0; i < rxValue.length(); i++) {              
              ledbuffer.push_back(rxValue[i]);
            }

            if (ledbuffer.size() == 167) {
              ackMB_L_LED();
              processLedBuffer();
              ledbuffer.clear();

            }

          }
          else if ((rxValue[0] & 0x7F) ==  MB_V_VERSION) {

            if (validateBMessage(rxValue)) {
              ackMB_V_VERSION();
            }
          }

          else if ((rxValue[0] & 0x7F) ==  MB_X_OFFLED) {
            if (validateBMessage(rxValue)) {
              ackMB_X_OFFLED();
            }

          }

          else if ((rxValue[0] & 0x7F) ==  MB_S_STATUS) {

            if (validateBMessage(rxValue)) {
              sendStatusState = 1;
              sendStatusIfNeeded();
            }
          }

          else if ((rxValue[0] & 0x7F) ==  MB_R_READ) {

            if (validateBMessage(rxValue)) {
              ackMB_R_READ(rxValue[1], rxValue[2]);
            }
          }

          else if ((rxValue[0] & 0x7F) ==  MB_W_WRITE) {

            if (validateBMessage(rxValue)) {
              ackMB_W_WRITE(rxValue[1], rxValue[2], rxValue[3], rxValue[4]);
            }

          }

          else if ((rxValue[0] & 0x7F) ==  MB_ADV_WRITE) {
            if (validateBMessage(rxValue, true)) {
              BLEdeviceConnected = false;
              startAdvertisement();
              offBLEConnectedIcon();
            }


          } else if ((rxValue[0] & 0x7F) ==  MB_XLED_WRITE) {

            if (validateBMessage(rxValue, true)) {

              if (serialLog)  Serial.println("X LEDS");

              if (rxValue.length() == 19) {

                for (int i = 0; i < 8; i++) {
                  byte ledData = decodeDataModeB(rxValue[2 * i + 1], rxValue[2 * i + 2]);

                  //if (serialLog)  Serial.print(ledData, HEX);
                  //if (serialLog)  Serial.print(" ");

                  for (int j = 0; j < 8; j++) {

                    ledStatusBufferX[(8 * i) + j] = bitRead(ledData, j);
                  }
                }

                //if (serialLog)  Serial.println("");

              } else {
                if (serialLog)  Serial.println("WRONG LED MESSAGE");
              }
            }

          }  else if ((rxValue[0] & 0x7F) ==  MB_UPDATE_INFO) {

            isDiablilloConnected=true;

            if (validateBMessage(rxValue, true)) {

              if (serialLog)  Serial.println("Requested update info");

              if (serialLog)  Serial.print("Update size: ");
              if (serialLog)  Serial.println(remoteUpdateFileSize, DEC);

              if (remoteUpdateFileSize > 0) {
                bleDeviceRequestedUpdateFile = true;

                if (serialLog)  Serial.print("Update CRC: ");
                if (serialLog)  Serial.println(remoteUpdateFileCRC);
              }

              ackMB_UPDATE_INFO(remoteUpdateFileSize, remoteUpdateFileCRC);
            }

          } else if ((rxValue[0] & 0x7F) ==  MB_UPDATE_READ) {

            if (validateBMessage(rxValue, true)) {

              if (serialLog)  Serial.println("Requested update file");

              sendRemoteUpdateFileOverBLE = true;
            }

          } else {
            if (serialLog)  Serial.print("UNKNOWN BLE Received Value LEN: ");
            if (serialLog)  Serial.print(rxValue.length(), DEC);
            if (serialLog)  Serial.print(" DATA: ");
            for (int i = 0; i < rxValue.length(); i++) {
              if (serialLog)  Serial.print(rxValue[i], HEX);
              if (serialLog)  Serial.print(" ");
            }

            if (serialLog) Serial.println(" ");
          }

        } //end bytes to process
      }
    }
};



uint16_t getBLEMTU() {
  return pServer->getPeerMTU(pServer->getConnId());
}


static boolean ledClientConnected = false;
static BLEAdvertisedDevice* myDevice;


//Callbacks for BLE connection and disconnection
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* servidor, esp_ble_gatts_cb_param_t *param) {


      if (ledClientConnected) {
        if (myDevice->getAddress().equals(BLEAddress(param->connect.remote_bda))) {
          if (serialLog) Serial.println("IGNORING CONNECT EVENT");
          return;
        }
      }

      if (serialLog)  Serial.println("BLE DEVICE CONNECTED");

      onBLEConnectedIcon();

      if (serialLog) Serial.print("BLE DEVICE MTU: ");
      if (serialLog) Serial.println(servidor->getPeerMTU(servidor->getConnId()), DEC);


      //clear led state boards
      for (int i = 0; i < 64 * 8; i++) {
        ledStatusBuffer[i] = 0;
        ledStatusBufferB[i] = 0;
        ledStatusBufferX[i] = 0;
      }
      //Initialize some stuff
      firstCharacterReceived = true;
      isModeALeds = false;

      //We will use command mode B
      BLEdeviceConnected = true;
      bleDeviceRequestedUpdateFile = false;
      isDiablilloConnected = false;
    };

    //For the client code to work I had to modify the disconnect server callback to include param data
    void onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {

      if (ledClientConnected) {
        if (myDevice->getAddress().equals(BLEAddress(param->disconnect.remote_bda))) {
          if (serialLog) Serial.println("IGNORING DISCONNECT EVENT");
          return;
        }
      }

      if (serialLog) Serial.println("BLE DEVICE DISCONNECTED");

      offBLEConnectedIcon();
      disableBLEmode = false;

      //We will use command mode A
      BLEdeviceConnected = false;

      //clear led state boards
      for (int i = 0; i < 64 * 8; i++) {
        ledStatusBuffer[i] = 0;
        ledStatusBufferB[i] = 0;
        ledStatusBufferX[i] = 0;
      }

      ledbuffer.clear();

      firstCharacterReceived = true;

      delay(500); // give the bluetooth stack the chance to get things ready

      startAdvertisement();

      bleDeviceRequestedUpdateFile = false;
      isDiablilloConnected=false;
    }
};


//Bluetooth BLE initialization for mode B boards

void initBle() {
  //UNCOMMENT FOR BLE DEGUG
  //Serial.setDebugOutput(true);
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.

  BLEDevice::init(modeBBtAdvertisedName);

  //Set BT power if specified by the user

  /*Accepts the following values:

    ESP_PWR_LVL_N12 = 0, Corresponding to -12dbm
    ESP_PWR_LVL_N9  = 1, Corresponding to  -9dbm
    ESP_PWR_LVL_N6  = 2, Corresponding to  -6dbm
    ESP_PWR_LVL_N3  = 3, Corresponding to  -3dbm
    ESP_PWR_LVL_N0  = 4, Corresponding to   0dbm
    ESP_PWR_LVL_P3  = 5, Corresponding to  +3dbm
    ESP_PWR_LVL_P6  = 6, Corresponding to  +6dbm
    ESP_PWR_LVL_P9  = 7, Corresponding to  +9dbm
  */

  boolean defaultPower = true;
  esp_power_level_t power = ESP_PWR_LVL_P3;
  if (strcmp(btPower, "0") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_N12;
  } else if (strcmp(btPower, "1") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_N9;
  } else if (strcmp(btPower, "2") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_N6;
  } else if (strcmp(btPower, "3") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_N3;
  } else if (strcmp(btPower, "4") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_N0;
  } else if (strcmp(btPower, "5") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_P3;
  } else if (strcmp(btPower, "6") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_P6;
  } else if (strcmp(btPower, "7") == 0) {
    defaultPower = false;
    power = ESP_PWR_LVL_P9;
  }

  if (!defaultPower) {
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_ADV, power);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_SCAN, power);
    esp_ble_tx_power_set(ESP_BLE_PWR_TYPE_DEFAULT, power);
    esp_bredr_tx_power_set(ESP_PWR_LVL_N0,  power);
  }

  int pwrAdv  = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV);
  int pwrScan = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
  int pwrDef  = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);

  if (serialLog) Serial.println("Power Settings: (ADV,SCAN,DEFAULT)");
  if (serialLog) Serial.println(pwrAdv);
  if (serialLog) Serial.println(pwrScan);
  if (serialLog) Serial.println(pwrDef);

  BLEDevice::setMTU(185);
}


void initBleService() {

  // Create the BLE Server
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // Create the BLE Service for Transparent UART Mode.
  pService = pServer->createService("49535343-fe7d-4ae5-8fa9-9fafd205e455");

  // Create a BLE Characteristic for TX data
  pTxCharacteristic = pService->createCharacteristic("49535343-1e4d-4bd9-ba61-23c647249616", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());

  // Create a BLE Characteristic for RX data
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic("49535343-8841-43f4-a8d4-ecbe34729bb3", BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new MyCallbacks());

  // Start the service
  pService->start();
  // Advertise the service

  startAdvertisement();


  //semaphore to handle data over BLE
  xSendDataBLESemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive( ( xSendDataBLESemaphore ) );
}



//Client section
//For the client code to work I had to modify the disconnect server callback to include param data

// The remote service we wish to connect to.
static BLEUUID serviceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");
// The characteristic of the remote service we are interested in.
static BLEUUID charUUID("49535343-8841-43f4-a8d4-ecbe34729bb3");

static BLERemoteCharacteristic* pRemoteCharacteristic;



static boolean clientDoConnect = false;
static boolean clientDoScan = false;


static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
  if (serialLog) Serial.print("Notify callback for characteristic");
  if (serialLog) Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  if (serialLog) Serial.print(" of data length ");
  if (serialLog) Serial.println(length);
  if (serialLog) Serial.print("data: ");
  if (serialLog) Serial.println((char*)pData);
}



class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient* pclient) {
      if (serialLog) Serial.println("BLE CLIENT CONNECTED");
    }

    void onDisconnect(BLEClient* pclient) {
      ledClientConnected = false;
      if (serialLog) Serial.println("BLE CLIENT DISCONNECTED");
    }
};


class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      //Serial.println("BLE Advertised Device found: ");
      //Serial.println(advertisedDevice.toString().c_str());
      //Serial.println(advertisedDevice.getServiceUUID().toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(BLEUUID("31f4"))) {

        //const uint8_t* point = esp_bt_dev_get_address();

        //Do not connect to ourselves
        if (advertisedDevice.getAddress() != BLEDevice::getAddress()) {

          BLEDevice::getScan()->stop();
          myDevice = new BLEAdvertisedDevice(advertisedDevice);
          clientDoConnect = true;
          clientDoScan = true;
        }
      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks



byte lastData[8];

void updateRemoteLeds(byte* data) {


  if (ledClientConnected) {

    if (memcmp(data, lastData , 8) == 0) {
      return;
    }

    //if (serialLog) Serial.println("SEND LED UPDATE");

    //byte command[9] = { MB_XLED_WRITE };

    //memcpy(&command[1], data, 8);

    memcpy(lastData, data, 8);

    //pRemoteCharacteristic->writeValue(command, 9);

    std::vector<byte> responseData;

    responseData.reserve(8);
    responseData.insert(responseData.begin(), data, data + 8);

    std::vector<byte> txValue = getBytesTransmitModeB(MB_XLED_WRITE, responseData);
    pRemoteCharacteristic->writeValue(txValue.data(), txValue.size());

  }

}



boolean connectToServer() {

  if (serialLog) Serial.println("Initializing scanning...");

  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);

  if (serialLog) Serial.println("Scanning for another module...");

  delay(2000);

  BLEDevice::getScan()->stop();

  if (clientDoConnect == false) {
    if (serialLog) Serial.println("Module not found");
    return false;
  }

  if (serialLog) Serial.print("Forming a connection to ");
  if (serialLog) Serial.println(myDevice->getAddress().toString().c_str());


  BLEClient*  pClient  = BLEDevice::createClient();
  if (serialLog) Serial.println(" - Created client");

  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server.
  pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
  if (serialLog) Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    if (serialLog) Serial.print("Failed to find our service UUID: ");
    if (serialLog) Serial.println(serviceUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  if (serialLog) Serial.println(" - Found our service");


  // Obtain a reference to the characteristic in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (pRemoteCharacteristic == nullptr) {
    if (serialLog) Serial.print("Failed to find our characteristic UUID: ");
    if (serialLog) Serial.println(charUUID.toString().c_str());
    pClient->disconnect();
    return false;
  }
  if (serialLog) Serial.println(" - Found our characteristic");

  //Notify the other module to advertise again
  std::vector<byte> responseData{};
  std::vector<byte> txValue = getBytesTransmitModeB(MB_ADV_WRITE, responseData);
  pRemoteCharacteristic->writeValue(txValue.data(), txValue.size());

  //Send fancy LED pattern
  std::vector<byte> responseData2{0xFF, 0x7E, 0x3C, 0x18, 0x18, 0x3C, 0x7E, 0xFF};
  std::vector<byte> txValue2 = getBytesTransmitModeB(MB_XLED_WRITE, responseData2);
  pRemoteCharacteristic->writeValue(txValue2.data(), txValue2.size());

  delay(2000);

  std::vector<byte> responseData3{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  std::vector<byte> txValue3 = getBytesTransmitModeB(MB_XLED_WRITE, responseData3);
  pRemoteCharacteristic->writeValue(txValue3.data(), txValue3.size());

  ledClientConnected = true;

  return true;
}



boolean updateRemoteLedsAvailable() {
  return ledClientConnected;
}
