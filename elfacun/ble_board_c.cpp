/*
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license

    This program is distributed WITHOUT ANY WARRANTY, even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "ble_board_c.h"

 


BLEServer *pServerC = NULL;
BLECharacteristic * pTxCharacteristicC;
BLECharacteristic * pTxCharacteristicC2;
BLECharacteristic * pTxCharacteristicC3;
BLEService *pServiceC;
BLEService *pServiceC2;
BLEService *pServiceC3;
BLEService *pServiceT;

uint8_t modeCmac[8];

boolean BLECdeviceConnected = false;


void startAdvertisementC() {

  pServerC->getAdvertising()->setAppearance(GENERIC_HID);

  BLEAdvertisementData oAdvertisementData = BLEAdvertisementData();
  BLEAdvertisementData oAdvertisementDataR = BLEAdvertisementData();
 
  std::string manufacturerData { 0x11, 0xFF, 0x50, 0x44, 0x43, 0x53, 0x5B, 0x54, 0x25, 0x64, 0x00, 0x00, modeCmac[5], modeCmac[4], modeCmac[3], modeCmac[2], modeCmac[1], modeCmac[0] }; 
  std::string txPower { 0x02, 0x0A, 0x0A }; 

  oAdvertisementDataR.setName(modeCBtAdvertisedName);
  oAdvertisementData.addData(manufacturerData);
  oAdvertisementData.addData(txPower);
  oAdvertisementData.setFlags(0x02 | 0x04);
  
  //oAdvertisementDataR.setAppearance(GENERIC_HID);  
  //oAdvertisementData.setPartialServices(mainID);
  //oAdvertisementDataR.setPartialServices(secondaryID);  
  
  pServerC->getAdvertising()->setAdvertisementData(oAdvertisementData);
  pServerC->getAdvertising()->setScanResponseData(oAdvertisementDataR);
  pServerC->getAdvertising()->setScanResponse(true);

  pServerC->getAdvertising()->start();

}

void stopAdvertisementC() {

  pServerC->getAdvertising()->stop();

}





//Callback for BLE data reception
class MyCallbacksC: public BLECharacteristicCallbacks {

    void onWrite(BLECharacteristic *pCharacteristic) {
      std::string rxValueRaw = pCharacteristic->getValue();

      //TBM SPLIT MULTIPLE COMMANDS INSIDE rxValue

      if (rxValueRaw.length() > 0) {

        int chunk = 1;

        if (serialDebug) {

          if (serialDebug) Serial.println(" ");
          if (serialDebug)  Serial.print("RECEIVED DATA LEN C: ");
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

          if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x21) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x29) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x1B) {

            bytesForCommand = 3;

          }else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x27) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x31) {

            bytesForCommand = 3;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x0B) {

            bytesForCommand = 6;

          } else if ((rxValueRaw[indexToProcess] & 0x7F) ==  0x0A) {

            bytesForCommand = 10;

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


          //Init board command
          if ((rxValue[0] & 0x7F) ==  0x21) {
            BLECdeviceConnected = true;
          }


          //LED command
          if ((rxValue[0] & 0x7F) ==  0x0A) {
            for (int i = 0; i< 64; i++) {
              for (int j = 0; j< 8; j++) {

                int index = (i / 8 * 8) + (7 - (i % 8));
                
                ledStatusBufferB[(64*j) + 63 - index ] = 4 * bitRead(rxValue[2 + (i / 8)] , (i % 8));
              }
            }
          }


          //ack
          std::vector<byte> txValue{0x23, 0x01, 0x00};

          if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
          {
            if (serialLog)  Serial.println("SEND C");
            pTxCharacteristicC2->setValue(txValue.data(), txValue.size());
            pTxCharacteristicC2->notify();
            if (serialLog)  Serial.println("SENT C");

            xSemaphoreGive( xSendDataBLESemaphore );
          }

        } //end bytes to process
      }
    }
};



uint16_t getBLEMTUC() {
  return pServerC->getPeerMTU(pServerC->getConnId());
}




//Callbacks for BLE connection and disconnection
class MyServerCallbacksC: public BLEServerCallbacks {
    void onConnect(BLEServer* servidor, esp_ble_gatts_cb_param_t *param) {


      if (serialLog)  Serial.println("BLE DEVICE C CONNECTED");

      onBLEConnectedIcon();

      if (serialLog) Serial.print("BLE DEVICE C MTU: ");
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

    };

    //For the client code to work I had to modify the disconnect server callback to include param data
    void onDisconnect(BLEServer* pServer, esp_ble_gatts_cb_param_t *param) {


      if (serialLog) Serial.println("BLE DEVICE C DISCONNECTED");

      offBLEConnectedIcon();
      disableBLEmode = false;

      //clear led state boards
      for (int i = 0; i < 64 * 8; i++) {
        ledStatusBuffer[i] = 0;
        ledStatusBufferB[i] = 0;
        ledStatusBufferX[i] = 0;
      }

      firstCharacterReceived = true;

      BLECdeviceConnected = false;

      delay(500); // give the bluetooth stack the chance to get things ready


      startAdvertisementC();

    }
};


//Bluetooth BLE initialization for mode C boards

void initBleC() {
  //UNCOMMENT FOR BLE DEGUG
  //Serial.setDebugOutput(true);
  //esp_log_level_set("*", ESP_LOG_VERBOSE);

  //Register and initialize BLE Transparent UART Mode.
  BLEDevice::init(modeCBtAdvertisedName);

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
    //esp_bredr_tx_power_set(ESP_PWR_LVL_N0,  power);
  }

  int pwrAdv  = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_ADV);
  int pwrScan = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_SCAN);
  int pwrDef  = esp_ble_tx_power_get(ESP_BLE_PWR_TYPE_DEFAULT);

  if (serialLog) Serial.println("Power Settings: (ADV,SCAN,DEFAULT)");
  if (serialLog) Serial.println(pwrAdv);
  if (serialLog) Serial.println(pwrScan);
  if (serialLog) Serial.println(pwrDef);

  //BLEDevice::setMTU(185);
}


void initBleServiceC() {

  // Create the BLE Server
  pServerC = BLEDevice::createServer();
  pServerC->setCallbacks(new MyServerCallbacksC());

  // Create the BLE Service for Transparent UART Mode.
  //For board data TX and board ACK
  pServiceC = pServerC->createService("1B7E8261-2877-41C3-B46E-CF057C562023");

  //For RX
  pServiceC2 = pServerC->createService("1B7E8271-2877-41C3-B46E-CF057C562023");

  


  // Create a BLE Characteristic for TX data
  pTxCharacteristicC = pServiceC->createCharacteristic("1B7E8262-2877-41C3-B46E-CF057C562023", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristicC->addDescriptor(new BLE2902());


  // Create a BLE Characteristic for RX data
  BLECharacteristic * pRxCharacteristicC = pServiceC2->createCharacteristic("1B7E8272-2877-41C3-B46E-CF057C562023", BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristicC->setCallbacks(new MyCallbacksC());
  

  // Create a BLE Characteristic for TX ack
  pTxCharacteristicC2 = pServiceC2->createCharacteristic("1B7E8273-2877-41C3-B46E-CF057C562023", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristicC2->addDescriptor(new BLE2902());


   //For ?
  pServiceC3 = pServerC->createService("1B7E8281-2877-41C3-B46E-CF057C562023");


  // Create a BLE Characteristic for RX data
  BLECharacteristic * pRxCharacteristicC3 = pServiceC3->createCharacteristic("1B7E8282-2877-41C3-B46E-CF057C562023", BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  pRxCharacteristicC3->setCallbacks(new MyCallbacksC());
  

  // Create a BLE Characteristic for TX ack
  pTxCharacteristicC3 = pServiceC3->createCharacteristic("1B7E8283-2877-41C3-B46E-CF057C562023", BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristicC3->addDescriptor(new BLE2902());




  //For TH
  pServiceT = pServerC->createService("9E5D1E47-5C13-43A0-8635-82AD38A1386F");

  // Create a BLE Characteristic for RX data
  BLECharacteristic * pRxCharacteristicT = pServiceT->createCharacteristic("E3DD50BF-F7A7-4E99-838E-570A086C666B", BLECharacteristic::PROPERTY_NOTIFY | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_INDICATE);
  pRxCharacteristicT->setCallbacks(new MyCallbacksC());
  pRxCharacteristicT->addDescriptor(new BLE2902());


  BLECharacteristic * pRxCharacteristicT2 = pServiceT->createCharacteristic("92E86C7A-D961-4091-B74F-2409E72EFE36", BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristicT2->setCallbacks(new MyCallbacksC());    

  // Create a BLE Characteristic for TX ack
  BLECharacteristic * pTxCharacteristicT = pServiceT->createCharacteristic("347F7608-2E2D-47EB-913B-75D4EDC4DE3B", BLECharacteristic::PROPERTY_READ);


  // Start the services
  pServiceC->start();
  pServiceC2->start();
  pServiceC3->start();
  pServiceT->start();
  
  // Advertise the services 
  startAdvertisementC();

  //semaphore to handle data over BLE
  xSendDataBLESemaphore = xSemaphoreCreateMutex();
  xSemaphoreGive( ( xSendDataBLESemaphore ) );
}



void sendPositionBLEBoardC() {

  std::vector<byte> txValueC(38, CEMPTY);

  txValueC[0] = 0x01;
  txValueC[1] = 0x24;

  for (int i = 0; i < 32; i++) {

    for (int j = 0; j < 2; j++) {

      int indexA = 2 * i + j;
      int index = (indexA / 8 * 8) + (7 - (indexA % 8));

      byte pieceValue = CEMPTY;

      switch (currentPos[index]) {
        case EMPTY:
          pieceValue = CEMPTY;
          break;
        case WPAWN:
          pieceValue = CWPAWN;
          break;
        case WROOK:
          pieceValue = CWROOK;
          break;
        case WKNIGHT:
          pieceValue = CWKNIGHT;
          break;
        case WBISHOP:
          pieceValue = CWBISHOP;
          break;
        case WKING:
          pieceValue = CWKING;
          break;
        case WQUEEN:
          pieceValue = CWQUEEN;
          break;
        case BPAWN:
          pieceValue = CBPAWN;
          break;
        case BROOK:
          pieceValue = CBROOK;
          break;
        case BKNIGHT:
          pieceValue = CBKNIGHT;
          break;
        case BBISHOP:
          pieceValue = CBBISHOP;
          break;
        case BKING:
          pieceValue = CBKING;
          break;
        case BQUEEN:
          pieceValue = CBQUEEN;
          break;
        default:
          pieceValue = CEMPTY;
          break;
      }
      if (j == 0) {
        txValueC[i + 2] = (txValueC[i + 2] & 0xF0) + pieceValue;
      } else {
        txValueC[i + 2] = (txValueC[i + 2] & 0x0F) + (pieceValue << 4);
      }
    }
  }

  //if (serialLog) Serial.print("Sending position mode C: ");
  //for (int i = 0; i< 38; i++) {
  //  if (serialLog) Serial.print(txValueC[i], HEX);
  //  if (serialLog) Serial.print(" ");
  //}
  //if (serialLog) Serial.println(" ");



  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    pTxCharacteristicC->setValue(txValueC.data(), txValueC.size());
    pTxCharacteristicC->notify();  

    xSemaphoreGive( xSendDataBLESemaphore );
  }


  return;
}
