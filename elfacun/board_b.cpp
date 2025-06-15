/*
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license

    This program is distributed WITHOUT ANY WARRANTY, even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "board_b.h"
#include "ble_board_c.h"

boolean hasToAckLed = false;

boolean disableStatusOnEveryScan = false;

boolean parityCheckDisabled = false;

byte setOddParity(byte in) {

  byte input = in;
  byte output = input & 0x7F;

  //  if (!BLEdeviceConnected) {
  //    return output;
  //  }
  //
  byte parity = 1;
  for (int i = 0; i < 8; i++) {
    byte bit = input & 0x01;
    input = input >> 1;
    parity = parity ^ bit;
  }
  if (parity) {
    return output | 0x80;
  }
  else {
    return output;
  }
}


byte firstHexDigit(byte input) {
  byte digit = input / 16 ;
  if (digit < 10) {
    return 0x30 + digit;
  } else {
    return 0x41 + digit - 10;
  }
}


byte secondHexDigit(byte input) {
  byte digit = input % 16;
  if (digit < 10) {
    return 0x30 + digit;
  } else {
    return 0x41 + digit - 10;
  }
}


std::vector<byte>  addCRC(std::vector<byte> input) {

  std::vector<byte> output(input.size() + 2, 0);

  byte blockParity = 0;
  for (int i = 0; i < input.size() ; i++) {
    output[i] = input[i];
    blockParity = blockParity ^ input[i] & 0x7F;
  }

  output[input.size()] = firstHexDigit(blockParity);
  output[input.size() + 1] = secondHexDigit(blockParity);
  return output;

}




byte decodeDataModeB(byte a, byte b) {
  byte i1 = a & 0x7F ;
  byte i2 = b & 0x7F ;

  byte acc = 0;

  if (i1 < 0x41) {
    acc += (i1 - 0x30) << 4;
  } else {
    acc += (i1 - 0x41 + 10) << 4;
  }

  if (i2 < 0x41) {
    acc += (i2 - 0x30);
  } else {
    acc += (i2 - 0x41 + 10);
  }
  return acc;
}





std::vector<byte>  getBytesTransmitModeB(byte command, std::vector<byte> data) {

  //Put everything in place without byte parity
  std::vector<byte> input(1 + (2 * data.size()) , 0);
  input[0] = command;
  for (int i = 0; i < data.size(); i++) {
    input[1 + (2 * i)] = firstHexDigit(data[i]);
    input[2 + (2 * i)] = secondHexDigit(data[i]);
  }

  //Generate block parity
  std::vector<byte> output = addCRC(input);

  //Generate byte parity
  //if (!BLEdeviceConnected) {
  for (int i = 0; i < output.size(); i++) {
    output[i] = setOddParity(output[i]);
  }
  //}
  return output;
}


boolean validateBMessage(std::string message) {
  return validateBMessage(message, false);
}


boolean validateBMessage(std::string message, boolean force) {

  if (parityCheckDisabled && !force) {
    return true;
  }

  if (message.size() < 3) {

    if (serialLog) {
      if (serialLog)  Serial.print("BLOCK CRC ERROR ON INCOMING B DATA: MESSAGE TOO SHORT: ");
      for (int i = 0; i < message.size()  ; i++) {
        if (serialLog)  Serial.print(message[i], HEX);
        if (serialLog)  Serial.print("-----------------------------");
      }

      if (serialLog) Serial.println("");
    }
    return false;
  }

  byte blockParity = 0;
  for (int i = 0; i < message.size() - 2 ; i++) {
    blockParity = blockParity ^ message[i] & 0x7F;
  }

  if ((message[message.size() - 2] & 0x7F) != firstHexDigit(blockParity) ||
      (message[message.size() - 1] & 0x7F) != secondHexDigit(blockParity)) {

    if (serialLog) {

      if (serialLog)  Serial.print("BLOCK CRC ERROR ON INCOMING B DATA: ");
      for (int i = 0; i < message.size()  ; i++) {
        if (serialLog) Serial.print(message[i], HEX);
        if (serialLog) Serial.print(" ");
      }

      if (serialLog) Serial.print(" RECEIVED CRC: ");
      if (serialLog) Serial.print((message[message.size() - 2] & 0x7F), HEX);
      if (serialLog) Serial.print(" ");
      if (serialLog) Serial.print((message[message.size() - 1] & 0x7F), HEX);
      if (serialLog) Serial.print(" ");

      if (serialLog) Serial.print(" CORRECT CRC: ");
      if (serialLog) Serial.print(firstHexDigit(blockParity), HEX);
      if (serialLog) Serial.print(" ");
      if (serialLog) Serial.print(secondHexDigit(blockParity), HEX);
      if (serialLog) Serial.print(" ");

      if (serialLog) Serial.println("-----------------------------");
    }
    return false;

  }
  return true;
}





std::vector<byte>  getBytesTransmitModeBNoEncode(byte command, std::vector<byte> data) {

  //Put everything in place without byte parity
  std::vector<byte> input(1 + data.size() , 0);
  input[0] = command;
  for (int i = 0; i < data.size(); i++) {
    input[1 +  i] = data[i];
  }

  //Generate block parity
  std::vector<byte> output = addCRC(input);

  //Generate byte parity
  //if (!BLEdeviceConnected) {
  for (int i = 0; i < output.size(); i++) {
    output[i] = setOddParity(output[i]);
  }
  //}
  return output;
}




void sendDataTypeBBoard(byte* data, int len) {

  //    if (serialLog)  Serial.print("SENDING DATA: ");
  //    for (int i = 0; i < len; i++) {
  //      if (serialLog)  Serial.print(data[i], HEX);
  //      if (serialLog) Serial.print(" ");
  //    }
  //
  //    if (serialLog) Serial.println("");

  if (BLEdeviceConnected) {
    //Send via bluetooth BLE
    pTxCharacteristic->setValue(data, len);
    pTxCharacteristic->notify();
  } else {

    writeSerial(data, len);
    commitSerial();
  }

}



void ackMB_V_VERSION() {


  if (isDiablilloConnected) {
    return;
  }



  std::vector<byte> responseData{1, 0};
  std::vector<byte> txValue = getBytesTransmitModeB(MB_V_VERSION + 0x20, responseData);


  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    sendDataTypeBBoard(txValue.data(), txValue.size());
    xSemaphoreGive( xSendDataBLESemaphore );
  }


}



void ackMB_R_READ(byte rxValue1, byte rxValue2) {



  std::vector<byte> responseData(2, 0);
  responseData[0] = decodeDataModeB(rxValue1, rxValue2);

  switch (responseData[0]) {
    case 0:
      //Serial port setup
      responseData[1] = parityCheckDisabled;
      break;
    case 1:
      //Board scan time
      responseData[1] = 30;
      break;
    case 2:
      //Automatic reports
      responseData[1] = updateModeTypeBUSBBoard;
      break;
    case 3:
      //Automatic status report time
      responseData[1] = updateIntervalTicksTypeBUSBBoard ;
      break;
    case 4:
      //LED brightness
      responseData[1] = 14;
      break;
    case 6:
      //No idea...
      responseData[1] = 0xEA;
    default:
      break;
  }


  if (isDiablilloConnected) {
    return;
  }


  std::vector<byte> txValue = getBytesTransmitModeB(MB_R_READ + 0x20, responseData);

  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    sendDataTypeBBoard(txValue.data(), txValue.size());
    xSemaphoreGive( xSendDataBLESemaphore );
  }

}



void ackMB_L_LED()
{

  if (isDiablilloConnected) {
    return;
  }

  hasToAckLed = true;
  ackMB_L_LED_Actually();
}


void ackMB_L_LED_Actually()
{
  hasToAckLed = false;

  std::vector<byte> responseData{};
  std::vector<byte> txValue = getBytesTransmitModeB(MB_L_LED + 0x20, responseData);

  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) 0 ) == pdTRUE )
  {
    sendDataTypeBBoard(txValue.data(), txValue.size());
    xSemaphoreGive( xSendDataBLESemaphore );
  }

  if (serialLog)  Serial.println("ACK LED");


}



void ackMB_X_OFFLED()
{



  //clear led state boards
  for (int i = 0; i < 64 * 8; i++) {
    ledStatusBufferB[i] = 0;
  }

  if (updateModeTypeBUSBBoard == 0) {
    //Update every scan
    sendStatusIfNeeded();
    return;
  }


  if (isDiablilloConnected) {
    return;
  }

  std::vector<byte> responseData{};
  std::vector<byte> txValue = getBytesTransmitModeB(MB_X_OFFLED + 0x20, responseData);

  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    sendDataTypeBBoard(txValue.data(), txValue.size());
    xSemaphoreGive( xSendDataBLESemaphore );
  }
}




void processLedBuffer() {

  ledUpdateReceived = true;

  //clear led state boards
  for (int i = 0; i < 64 * 8; i++) {
    ledStatusBufferB[i] = 0;
  }


  for (int slot = 0; slot < 8; slot++) {

    int numlit = 0;
    std::vector<byte> col2500Mode(8, 0x00);
    std::vector<byte> row2500Mode(8, 0x00);
    boolean onlyFirstColLastRowLit = true;
    int numledsFirstCol = 0;
    int numledsLastCol = 0;
    int numledsFirstRow = 0;
    int numledsLastRow = 0;
    boolean is2500CableRight = false;
    boolean is2500CableLeft = false;

    //Preprocess for RISC 2500 mode
    for (int numled = 0; numled < 81; numled++) {

      byte ledPattern = decodeDataModeB(ledbuffer.at(3 + (2 * numled)), ledbuffer.at(3 + (2 * numled) + 1));
      byte numledTransposed = (numled % 9) * 9 + (numled / 9);

      boolean lit = bitRead(ledPattern, slot);

      if (lit) {

        if ((numled / 9) < 8 && (numled / 9) > 0 && (numled % 9) > 0 && (numled % 9) < 8) {
          onlyFirstColLastRowLit = false;
        }

        if (numled / 9 == 0) {
          numledsFirstCol++;
        }
        if (numled / 9 == 8) {
          numledsLastCol++;
        }
        if (numled % 9 == 0) {
          numledsFirstRow++;
        }
        if (numled % 9 == 8) {
          numledsLastRow++;
        }
      }
    }

    if (numledsFirstCol >= 2 && numledsLastRow >= 2 && onlyFirstColLastRowLit) {
      is2500CableLeft = true;
      if (serialLog) Serial.println("RISC 2500 CABLE LEFT");
    }

    if (numledsLastCol >= 2 && numledsFirstRow >= 2 && onlyFirstColLastRowLit)  {
      is2500CableRight = true;
      if (serialLog) Serial.println("RISC 2500 CABLE RIGHT");
    }

    //Store ledbuffer in the materialized led array, that is 8 consecutive led state boards that will be cycled
    for (int numled = 0; numled < 81; numled++) {

      //Each led potentially affects up to 4 squares. If all the leds for a sqare are lit then we lit the led in the boad.
      //Obviously we don't have as many leds as modern boards to reproduce all the possible patterns

      byte ledPattern = decodeDataModeB(ledbuffer.at(3 + (2 * numled)), ledbuffer.at(3 + (2 * numled) + 1));
      byte numledTransposed = (numled % 9) * 9 + (numled / 9);

      //Extract state for each board

      boolean lit = bitRead(ledPattern, slot);

      if (lit) {
        numlit++;
        boolean firstColLit = false;
        boolean lastRowLit = false;

        if ((numled / 9 == 8 && is2500CableRight) || (numled / 9 == 0 && is2500CableLeft)) {
          firstColLit = true;
        }

        if ((numled % 9 == 0 && is2500CableRight) || (numled % 9 == 8 && is2500CableLeft)) {
          lastRowLit = true;
        }

        //set affected squares
        //bottom right
        if ((numledTransposed < 72) && ((numledTransposed % 9) != 8)) {
          ledStatusBufferB[(slot * 64) + numledTransposed - (numledTransposed / 9)] = ledStatusBufferB[(slot * 64) + numledTransposed - (numledTransposed / 9)] + 1;
        }

        //bottom left
        if ((numledTransposed < 72) && ((numledTransposed % 9) != 0)) {
          ledStatusBufferB[(slot * 64) + numledTransposed - 1 - (numledTransposed / 9)] = ledStatusBufferB[(slot * 64) + numledTransposed - 1 - (numledTransposed / 9)] + 1;
        }

        //top right
        if ((numledTransposed > 8) && ((numledTransposed % 9) != 8)) {
          ledStatusBufferB[(slot * 64) + numledTransposed - 9 - (numledTransposed / 9) + 1] = ledStatusBufferB[(slot * 64) + numledTransposed - 9 - (numledTransposed / 9) + 1]  + 1;
        }

        //top left
        if ((numledTransposed > 8) && ((numledTransposed % 9) != 0)) {
          ledStatusBufferB[(slot * 64) + numledTransposed - 10 - (numledTransposed / 9) + 1] = ledStatusBufferB[(slot * 64) + numledTransposed - 10 - (numledTransposed / 9) + 1] + 1;
        }

        //row top (RISC mode)
        if (firstColLit && numled % 9 > 0) {
          row2500Mode[numled % 9 - 1] = row2500Mode[numled % 9 - 1] + 1;
        }

        //row bottom (RISC mode)
        if (firstColLit && numled % 9 < 8) {
          row2500Mode[numled % 9] = row2500Mode[numled % 9] + 1;
        }

        //col left (RISC mode)
        if (lastRowLit && numled / 9 > 0) {
          col2500Mode[numled / 9 - 1] = col2500Mode[numled / 9 - 1] + 1;
        }

        //col right (RISC mode)
        if (lastRowLit && numled / 9 < 8) {
          col2500Mode[numled / 9] = col2500Mode[numled / 9] + 1;
        }
      }
    }

    if ((numlit == 3 || numlit == 4) && onlyFirstColLastRowLit) {
      //RISC 2500 mode
      byte colLit2500 = 0x00;
      byte rowLit2500 = 0x00;
      int numColLit = 0;
      int numRowLit = 0;
      for (int i = 0; i < 8; i++) {
        //check if only one col and row is lit and lit the corresponding square
        if (col2500Mode[i] == 2 && (is2500CableLeft || !numColLit)) {
          colLit2500 = i;
          numColLit ++;
        }
        if (row2500Mode[i] == 2 && (is2500CableRight || !numRowLit)) {
          rowLit2500 = i;
          numRowLit ++;
        }
      }
      if (numRowLit && numColLit) {
        ledStatusBufferB[(slot * 64) + (rowLit2500 * 8) + colLit2500] = 4;
      }
    }
  }
}



void ackMB_W_WRITE(byte rxValue1, byte rxValue2, byte rxValue3, byte rxValue4) {


  std::vector<byte> responseData(2, 0);

  //fake
  responseData[0] = decodeDataModeB(rxValue1, rxValue2);
  responseData[1] = decodeDataModeB(rxValue3, rxValue4);

  //Store value

  switch (responseData[0]) {
    case 0:
      //Serial port setup
      parityCheckDisabled = responseData[1] & 0x01;
      if (serialLog)  Serial.print("SETTING Serial port parity: ");
      if (serialLog)  Serial.println(parityCheckDisabled, DEC);
      break;
    case 1:
      //Board scan time
      if (serialLog)  Serial.println("SETTING Board scan time (NOT IMPLEMENTED) ");
      break;
    case 2:
      //Automatic reports
      updateModeTypeBUSBBoard = responseData[1];
      if (serialLog)  Serial.print("SETTING updateModeTypeBUSBBoard: ");
      if (serialLog)  Serial.println(updateModeTypeBUSBBoard, DEC);
      break;
    case 3:
      //Automatic status report time
      updateIntervalTicksTypeBUSBBoard = responseData[1];
      updateIntervalTypeBUSBBoard = 4 * responseData[1];
      if (serialLog)  Serial.print("SETTING updateIntervalTicksTypeBUSBBoard: ");
      if (serialLog)  Serial.println(updateIntervalTicksTypeBUSBBoard, DEC);
      break;
    case 4:
      //LED brightness
      if (serialLog)  Serial.println("SETTING LED brightness (NOT IMPLEMENTED) ");
      break;
    default:
      if (serialLog)  Serial.println("SETTING Unknown variable (NOT IMPLEMENTED) ");
      break;
  }



  if (isDiablilloConnected) {
    return;
  }

  std::vector<byte> txValue = getBytesTransmitModeB(MB_W_WRITE + 0x20, responseData);

  if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
  {
    sendDataTypeBBoard(txValue.data(), txValue.size());
    xSemaphoreGive( xSendDataBLESemaphore );
  }

}




void sendStatusIfNeeded() {

  if ((BLECdeviceConnected || BLEdeviceConnected || (isTypeBUSBBoard && !firstCharacterReceived)) && ! bleDeviceRequestedUpdateFile) {

    if (BLECdeviceConnected) {
      sendPositionBLEBoardC();
      return;
    }

    std::vector<byte> responseData(64, BEMPTY);

    for (int i = 0; i < 64; i++) {

      int index = 63 - i;

      //      if (boardInverted) {
      //        index = 63 - i;
      //      }

      switch (currentPos[index]) {
        case EMPTY:
          responseData[i] = BEMPTY;
          break;
        case WPAWN:
          responseData[i] = BWPAWN;
          break;
        case WROOK:
          responseData[i] = BWROOK;
          break;
        case WKNIGHT:
          responseData[i] = BWKNIGHT;
          break;
        case WBISHOP:
          responseData[i] = BWBISHOP;
          break;
        case WKING:
          responseData[i] = BWKING;
          break;
        case WQUEEN:
          responseData[i] = BWQUEEN;
          break;
        case BPAWN:
          responseData[i] = BBPAWN;
          break;
        case BROOK:
          responseData[i] = BBROOK;
          break;
        case BKNIGHT:
          responseData[i] = BBKNIGHT;
          break;
        case BBISHOP:
          responseData[i] = BBBISHOP;
          break;
        case BKING:
          responseData[i] = BBKING;
          break;
        case BQUEEN:
          responseData[i] = BBQUEEN;
          break;
        default:
          responseData[i] = BEMPTY;
          break;
      }
    }

    std::vector<byte> txValue = getBytesTransmitModeBNoEncode(MB_S_STATUS + 0x20, responseData);




    if (BLEdeviceConnected) {

      //Slip vector in chunks if BLE to force Android to work
      int n = getBLEMTU() - 3;

      int size = (txValue.size() - 1) / n + 1;
      std::vector<byte> vec[size];
      for (int k = 0; k < size; ++k)
      {
        auto start_itr = std::next(txValue.cbegin(), k * n);
        auto end_itr = std::next(txValue.cbegin(), k * n + n);
        vec[k].resize(n);
        if (k * n + n > txValue.size())
        {
          end_itr = txValue.cend();
          vec[k].resize(txValue.size() - k * n);
        }
        std::copy(start_itr, end_itr, vec[k].begin());
      }

      //Send the chunks
      if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
      {
        for (int i = 0; i < size; i++) {
          //        delay(10);
          sendDataTypeBBoard(vec[i].data(), vec[i].size());
          //        delay(10);
          //if (serialLog)  Serial.print("BLE Sent Value MB_S_STATUS size: ");
          //if (serialLog)  Serial.println(vec[i].size());

        }
        xSemaphoreGive( xSendDataBLESemaphore );
      }

    } else {
      //Send the whole data
      if ( xSemaphoreTake( xSendDataBLESemaphore, ( TickType_t ) portMAX_DELAY ) == pdTRUE )
      {
        sendDataTypeBBoard(txValue.data(), txValue.size());
        //if (serialLog)  Serial.print("BLE Sent Value MB_S_STATUS size: ");
        //if (serialLog)  Serial.println(txValue.size());
        xSemaphoreGive( xSendDataBLESemaphore );
      }

    }

  }
}


//Detect incoming commands and launch required actions
void board_b_command_loop(byte rcv) {
  //Type B board
  byte par1;
  byte par2;
  byte par3;
  byte par4;
  byte crc1;
  byte crc2;
  switch (rcv & 0x7F) {

    case MB_S_STATUS:
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      sendStatusIfNeeded();
      break;

    case MB_L_LED:
      ledbuffer.clear();
      ledbuffer.push_back(rcv);
      byte ledDataBuffer[164];
      readSerial(ledDataBuffer, 164);
      for (int i = 0; i < 164; i++) {
        ledbuffer.push_back(ledDataBuffer[i]);
      }
      ledbuffer.push_back(readSerialBlock());
      ledbuffer.push_back(readSerialBlock());

      ackMB_L_LED();
      processLedBuffer();
      ledbuffer.clear();
      break;

    case MB_X_OFFLED:
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      ackMB_X_OFFLED();
      break;

    case MB_T_RESET:
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      reset();
      break;

    case MB_V_VERSION:
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      ackMB_V_VERSION();
      break;

    case MB_W_WRITE:
      par1 = readSerialBlock();
      par2 = readSerialBlock();
      par3 = readSerialBlock();
      par4 = readSerialBlock();
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      ackMB_W_WRITE(par1, par2, par3, par4);
      break;

    case MB_R_READ:
      par1 = readSerialBlock();
      par2 = readSerialBlock();
      crc1 = readSerialBlock();
      crc2 = readSerialBlock();
      ackMB_R_READ(par1, par2);
      break;

    default:
      if (serialLog)  Serial.print("RECEIVED UNKNOWN MODE B OPCODE: ");
      if (serialLog)  Serial.println(rcv);
      break;
  }


  if (serialLog) Serial.print("Received command: ");
  if (serialLog) Serial.println(rcv, HEX);

}
