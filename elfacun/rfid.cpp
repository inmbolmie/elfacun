/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

#include "rfid.h"

#define RST_PIN         13
#define SS_PIN_RFID          12

MFRC522 mfrc522(SS_PIN_RFID, RST_PIN);   // Create MFRC522 instance.

MFRC522::MIFARE_Key key;

void rfid_init() {
  mfrc522.PCD_Init(); // Init MFRC522 card  

  // Prepare the key (used both as key A and as key B)
  // using FFFFFFFFFFFFh which is the default at chip delivery from the factory
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}


boolean programRfidTag(int piece) {

  //    if (serialLog) Serial.print(F("PROGRAMMING MODE FOR PIECE: "));
  //    if (serialLog) Serial.println(missingPiece, DEC);

  byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
  byte size = sizeof(buffer);
  uint8_t pageAddr = 0x06;

  MFRC522::StatusCode status; //variable to get card status
  memcpy(buffer, "  ELFACUN       ", 16);
  buffer[0] = piece;
  if ( mfrc522.PICC_IsNewCardPresent())
  {

    // Select one of the cards
    if ( mfrc522.PICC_ReadCardSerial())
    {

      // Write data ***********************************************
      for (int i = 0; i < 4; i++) {
        //data is writen in blocks of 4 bytes (4 bytes per page)
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Ultralight_Write(pageAddr + i, &buffer[i * 4], 4);
        if (status != MFRC522::STATUS_OK) {
          if (serialLog) Serial.print(F("MIFARE_Read() failed: "));
          if (serialLog) Serial.println(mfrc522.GetStatusCodeName(status));
          break;
        }
      }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();
      return true;
    }

    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1();
  }

  return false;
}





boolean readRfidTag(byte* liftedPiece) {
  if ( mfrc522.PICC_IsNewCardPresent() ) {
    if (mfrc522.PICC_ReadCardSerial()) {
      MFRC522::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);

      // Check for compatibility
      if (    piccType == MFRC522::PICC_TYPE_MIFARE_MINI
              ||  piccType == MFRC522::PICC_TYPE_MIFARE_1K
              ||  piccType == MFRC522::PICC_TYPE_MIFARE_4K) {


        // that is: sector #1, covering block #4 up to and including block #7
        byte sector         = 1;
        byte blockAddr      = 4;
        byte trailerBlock   = 7;
        MFRC522::StatusCode status;
        byte buffer[18];
        byte size = sizeof(buffer);

        // Read tag
        status = (MFRC522::StatusCode) mfrc522.PCD_Authenticate(MFRC522::PICC_CMD_MF_AUTH_KEY_A, trailerBlock, &key, &(mfrc522.uid));
        if (status == MFRC522::STATUS_OK) {
          // Show the whole sector as it currently is
          //if (serialLog) Serial.print(F("TAG ID:"));
          byte buffer[18];
          byte size = sizeof(buffer);
          mfrc522.MIFARE_Read(4, buffer, &size);
          if (serialLog) Serial.println(buffer[0], HEX);
          *liftedPiece = buffer[0];
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          return true;
        }
      }

      else if (piccType == MFRC522::PICC_TYPE_MIFARE_UL) {
        MFRC522::StatusCode status;
        byte buffer[18];  //data transfer buffer (16+2 bytes data+CRC)
        byte size = sizeof(buffer);
        uint8_t pageAddr = 0x06;
        status = (MFRC522::StatusCode) mfrc522.MIFARE_Read(pageAddr, buffer, &size);
        if (status == MFRC522::STATUS_OK) {
          if (serialLog) Serial.print("XXXXXX____LEIDA: ");
          if (serialLog) Serial.println(buffer[0], HEX);
          *liftedPiece = buffer[0];
          mfrc522.PICC_HaltA();
          mfrc522.PCD_StopCrypto1();
          return true;
        }


      }
      mfrc522.PICC_HaltA();
      mfrc522.PCD_StopCrypto1();

    }
  }

  return false;
}
