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


//Callback for BLE data reception
class MyCallbacks: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValue = pCharacteristic->getValue();

      if (rxValue.length() > 0) {

//        if (serialLog) Serial.println(" ");
//        if (serialLog)  Serial.print("RECEIVED DATA LEN: ");
//        if (serialLog)  Serial.print(rxValue.length(), DEC);
//        if (serialLog)  Serial.print(" DATA: ");
//        for (int i = 0; i < rxValue.length(); i++) {
//          if (serialLog)  Serial.print(rxValue[i], HEX);
//          if (serialLog)  Serial.print(" ");
//        }
//
//        if (serialLog) Serial.println(" ");

        //check if receiving a led command
        if (!ledbuffer.empty()) {

          for (int i = 0; i < rxValue.length(); i++) {
            ledbuffer.push_back(rxValue[i]);
          }

          if (ledbuffer.size() == 167) {
            ackMB_L_LED();
            processLedBuffer();
            ledbuffer.clear();

          }
          else if (ledbuffer.size() > 167) {
            //Error
            ledbuffer.clear();
            if (serialLog)  Serial.println("ERROR RECEIVING LED INFO");
          }

        }

        //response
        else if ((rxValue[0] & 0x7F) ==  MB_L_LED) {

          //ledbuffer.clear();
          //ackMB_L_LED();

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

          ackMB_V_VERSION();
        }

        else if ((rxValue[0] & 0x7F) ==  MB_X_OFFLED) {

          ackMB_X_OFFLED();
        }

        else if ((rxValue[0] & 0x7F) ==  MB_S_STATUS) {
          sendStatusState = 1;
          sendStatusIfNeeded();
        }

        else if ((rxValue[0] & 0x7F) ==  MB_R_READ) {

          ackMB_R_READ(rxValue[1], rxValue[2]);
        }

        else if ((rxValue[0] & 0x7F) ==  MB_W_WRITE) {

          ackMB_W_WRITE(rxValue[1], rxValue[2], rxValue[3], rxValue[4]);

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
      }
    }
};



uint16_t getBLEMTU() {
  return pServer->getPeerMTU(pServer->getConnId());
}


//Callbacks for BLE connection and disconnection
class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* servidor) {
      if (serialLog)  Serial.println("BLE DEVICE CONNECTED");


      if (serialLog) Serial.print("BLE DEVICE MTU: ");
      if (serialLog) Serial.println(servidor->getPeerMTU(servidor->getConnId()), DEC);


      //clear led state boards
      for (int i = 0; i < 64 * 8; i++) {
        ledStatusBuffer[i] = 0;
      }
      //Initialize some stuff
      firstCharacterReceived = true;
      isModeALeds = false;

      //We will use command mode B
      BLEdeviceConnected = true;
    };

    void onDisconnect(BLEServer* pServer) {
      if (serialLog) Serial.println("BLE DEVICE DISCONNECTED");
      //We will use command mode A
      BLEdeviceConnected = false;

      //clear led state boards
      for (int i = 0; i < 64 * 8; i++) {
        ledStatusBuffer[i] = 0;
      }

      firstCharacterReceived = true;

      delay(500); // give the bluetooth stack the chance to get things ready
      //pServer->startAdvertising(); // restart advertising

      pServer->getAdvertising()->setAppearance(GENERIC_HID);

      pServer->getAdvertising()->addServiceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");

      pServer->getAdvertising()->setScanResponse(true);

      BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

      oAdvertisementData.setShortName(modeBBtAdvertisedName);
      oAdvertisementData.setName(modeBBtAdvertisedName);
      oAdvertisementData.setAppearance(GENERIC_HID);

      pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);

      pServer->getAdvertising()->start();
    }
};


//Bluetooth BLE initialization for mode B boards
void initBleService() {

  //UNCOMMENT FOR BLE DEGUG
  //Serial.setDebugOutput(true);
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.

  BLEDevice::init(modeBBtAdvertisedName);

  BLEDevice::setMTU(185);

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

  pServer->getAdvertising()->setAppearance(GENERIC_HID);

  pServer->getAdvertising()->addServiceUUID("49535343-fe7d-4ae5-8fa9-9fafd205e455");

  pServer->getAdvertising()->setScanResponse(true);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();

  oAdvertisementData.setShortName(modeBBtAdvertisedName);
  oAdvertisementData.setName(modeBBtAdvertisedName);
  oAdvertisementData.setAppearance(GENERIC_HID);

  pServer->getAdvertising()->setAdvertisementData(oAdvertisementData);

  pServer->getAdvertising()->start();


  //semaphore to handle data over BLE
  xSendDataBLESemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive( ( xSendDataBLESemaphore ) );
}
