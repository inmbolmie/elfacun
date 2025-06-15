/*  
    ElFacun Chess Module software for ESP32
    © 2022 Inmbolmie inmbolmie [at] gmail [dot] com
    Distributed under the GNU GPL V3.0 license
    
    This program is distributed WITHOUT ANY WARRANTY, even the 
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
    PURPOSE.  See the GNU General Public License for more details.
*/

//ESP32 I/O PORTS ASSIGNMENTS
//00 Audio Jack R (I2C master)

//02 Buzzer
//04 CONTROLLER OUTPUT CS
//05 CONTROLLER OUTPUT DISABLE

//12 RFID CS
//13 RFID RESET
//14 SPI RESET 3V
//15 DISPLAY D/C 3V
//16 Audio Jack L (I2C master)
//17 DISPLAY CS
//18 SPI SCK 3V
//19 SPI SO 3V

//21 Audio Jack R (I2C slave)
//22 Audio Jack L (I2C slave)
//23 SPI SI 3V

//25 ENABLE BUS SWITCHES
//26 SD CARD CS
//27 CONTROLLER OUTPUT BUTTONS CS

//32 LATCH DISABLE B (columns)
//33 LATCH DISABLE A (rows)


//Board interface control pins

//RFID
#define RFID_CS 12

//SD INTERFACE
#define SD_CS 26

//ENABLE BUS SWITCHES
#define EN_BUS 25

//Data from controller to board
#define CONTROLLER_OUTPUT_DISABLE 5
#define CONTROLLER_OUTPUT_CHIPSELECT 4
#define CONTROLLER_OUTPUT_CHIPSELECT_BUTTONS 27


//Board latches controlled through 40-pin connector with the 23S17. So the numbers are 23S17 port numbers
//BUS_29
#define LED_WR_DISABLE_COLUMN_LOAD_EX 8
//BUS_32
#define LED_WR_LOAD_EX 9
//BUS_31
#define ROW_LOAD_EX 10
//BUS_30
#define COLUMN_DISABLE_EX 11
//DISPLAY CONTROL
#define DISPLAY_CTRL_A 12
#define DISPLAY_CTRL_B 13
#define DISPLAY_CTRL_C 14

//Special data latches for passive mode
#define LATCH_ROWS_DISABLE 33
#define LATCH_COLUMNS_DISABLE 32

//probe to get passive latch inhibition state
#define INHIB_INV 7

//Buzzer pin
#define BUZZER 2


//ELFACUN2 SPECIFIC PINS

#define HARDWARE_V2_DETECT 6
#define MEM_WRITE_ENABLE 7  // on V1 is INHIB_INV
#define MEM_ADDR_0 12 // on V1 is DISPLAY_CTRL_A 0
#define MEM_ADDR_1 13 // on V1 is DISPLAY_CTRL_B 1
#define MEM_ADDR_2 14 // on V1 is DISPLAY_CTRL_C 2
#define MEM_ADDR_SRC 15 // on V1 is NC

#define WRITE_ENABLE_SAFE 33 // on V1 is LATCH_ROWS_DISABLE

#define MEM_ADDR_SRC_BUS true
#define MEM_ADDR_SRC_PIC false

#define SCREEN_BRIGHTNESS 32 // on V1 is LATCH_COLUMNS_DISABLE
 
