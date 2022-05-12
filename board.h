#ifndef BOARD_H_
#define BOARD_H_

#define SENSE_PIN A7

// #define BOARD_ID "namco ltd.;RAYS PCB;Ver1.06;JPN,I/O"
#define BOARD_ID "Burwenzel Incorporated Special I/O"

#define BOARD_CMD_VER 0x11
#define BOARD_JVS_VER 0x20
#define BOARD_COM_VER 0x10

#define BOARD_PLAYERS 1     // total players
#define BOARD_BUTTONS 0x0C  // switches per player

#define BOARD_BUTBANKS (int)ceil(BOARD_BUTTONS/8.0) // Button banks per player

#define BOARD_COINSLOTS 1

#define BOARD_GUN_RESBITS_X 0x10
#define BOARD_GUN_RESBITS_Y 0x10
#define BOARD_GUN_CHANNELS 1

#define BOARD_GPO_BANKS 2
#define BOARD_GPI_BANKS 0

#define BOARD_ANALOG_IN 0
#define BOARD_ANALOG_RESBITS 0



typedef struct {
  int8_t swBank[BOARD_BUTBANKS];
} _swPlayerBank;

typedef struct {
   int16_t gun_pos_x;
   int16_t gun_pos_y;
} gunPosData; 

typedef struct {
   int8_t b1;
   int8_t b2;
} coinSlot; 

typedef struct {
  uint8_t boardAddress;
  uint8_t swSystem;
  _swPlayerBank swPlayerBank[BOARD_PLAYERS];
  coinSlot coinSlots[BOARD_COINSLOTS];
  gunPosData lightGuns[BOARD_GUN_CHANNELS];
} boardIOdata;


#endif // BOARD_H_
