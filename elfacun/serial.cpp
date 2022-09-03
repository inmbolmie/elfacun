/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "serial.h"
#include "Arduino.h"
#include "lichess.h"


BluetoothSerial SerialBT;
boolean btConnected = false;

byte serialCommandBuffer[100];
byte serialCommandBufferLen = 0;


void commitSerial() {

  if (!isLichessConnected && btConnected) {
    SerialBT.write(serialCommandBuffer, serialCommandBufferLen);
  }

  if (Serial) {
    if (serialDebug || serialLog) {
      if (serialDebug) Serial.print("Sending data: ");
      for (int i = 0; i < serialCommandBufferLen; i++) {
        if (serialDebug) Serial.print(serialCommandBuffer[i], HEX);
        if (serialDebug) Serial.print(" ");
      }
      if (serialDebug) Serial.println("");

    } else {
      if (!btConnected || serialMirror) {
        Serial.write(serialCommandBuffer, serialCommandBufferLen);
      }
    }
  }

  serialCommandBufferLen = 0;

}


byte readSerial() {

  if (!isLichessConnected && SerialBT.available()) {
    return SerialBT.read();
  } else {
    return Serial.read();
  }

}



byte readSerialBlock() {

  int timeout = millis() + 1000;

  do {
    if (SerialBT.available()) {
      return SerialBT.read();
    }
    if (Serial.available())  {
      return Serial.read();
    }
  } while (millis() < timeout);

}
  


void readSerial(byte* buffer, int len) {


  if (!isLichessConnected && SerialBT.available()) {
    SerialBT.readBytes(buffer, len);
  } else {
    Serial.readBytes(buffer, len);
  }

}



void writeSerial(byte data) {

  serialCommandBuffer[serialCommandBufferLen++] = data;

}


void writeSerial(byte* data, int len) {

  memcpy(&serialCommandBuffer[serialCommandBufferLen] , data, len);
  serialCommandBufferLen += len;

}


void printSerial(String data) {

  memcpy(&serialCommandBuffer[serialCommandBufferLen] , data.c_str(), data.length());
  serialCommandBufferLen += data.length();



}




void printSerial(String data, int len) {

  int dataLenIndex = data.length() ;
  int requiredLenIndex = len ;

  memcpy(&serialCommandBuffer[serialCommandBufferLen] , data.c_str(), min(dataLenIndex, requiredLenIndex));
  serialCommandBufferLen += data.length();

  for (int i = data.length(); i < len; i++) {
    serialCommandBuffer[serialCommandBufferLen++] = '0';
  }


}
