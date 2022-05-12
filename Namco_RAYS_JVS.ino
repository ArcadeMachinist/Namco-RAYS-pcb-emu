#include <ArduinoJson.h>
#include "jvs.h"
#include "board.h"
#include "marcos.h"

boardIOdata io_board;

unsigned long LED_R1_timing = 0;

void setup() {
  pinMode(SENSE_PIN, INPUT);

  
  pinMode(8, INPUT);
  pinMode(11, INPUT);
  pinMode(12, INPUT);
  pinMode(13, INPUT);


  // Led Bar
  pinMode(23,OUTPUT);
 // digitalWrite(23, HIGH);
  LED_R1_timing = millis();
  
  Serial.begin(115200);
  delay(1000);
  Serial.println("Hello");
  Serial1.begin(115200);
  Serial.println("Fin");
}

void loop() {
  // put your main code here, to run repeatedly:
  if ((millis() - LED_R1_timing) > 1000) {
    if (digitalRead(23)) { 
        digitalWrite(23, LOW);
    } else {
        digitalWrite(23, HIGH);
    }
    LED_R1_timing = millis();
  }
  
  JVS_Serial_Receive();
  Console_Receive();
  PoorManGun();
}

void PoorManGun() {
    io_board.lightGuns[0].gun_pos_x = map (analogRead(A0),0,1023,0,0x3FF);
    io_board.lightGuns[0].gun_pos_y = map (analogRead(A1),0,1023,0,0xFF);
    BIT_WRITE(io_board.swPlayerBank[0].swBank[0],0,digitalRead(8));

    BIT_WRITE(io_board.swPlayerBank[0].swBank[0],4,digitalRead(11));
    BIT_WRITE(io_board.swPlayerBank[0].swBank[0],5,digitalRead(12));
    BIT_WRITE(io_board.swPlayerBank[0].swBank[0],1,digitalRead(13));
    
}

void Console_Receive() {
  int incomingByte;
  if (Serial.available() > 0) {
    incomingByte = Serial.read();
        if (incomingByte != -1) {
          switch(incomingByte) {
            case 0x74:
              Serial.println("OK");
              if (BIT_READ(io_board.swSystem,7)) {
                BIT_CLEAR(io_board.swSystem,7);
              } else {
                BIT_SET(io_board.swSystem,7);
              }
            default:
              Serial.println("NG");
          }
          
        }
  }
}

void JVS_Serial_Receive() {
  byte jvs_msg[128];
  byte jvs_phase = 0;
  byte jvs_escape = 0;
  byte jvs_checksum = 0;
  byte jvs_read = 0;
  byte jvs_dest = 0;
  byte jvs_length = 0;
  int incomingByte;
  
  if (Serial1.available() > 0) {
    // Serial.println("AV>0");
     while(true) {
        incomingByte = Serial1.read();
        if (incomingByte != -1) {
        if (incomingByte == SYNC && !jvs_escape) {
          //Serial.println("Got E0");
          jvs_phase = 1;
          jvs_checksum = 0;
          jvs_read = 0;
          memset(jvs_msg, 0, sizeof(jvs_msg));
          continue;
        } else if (incomingByte == ESCAPE
        && !jvs_escape) {
          jvs_escape = 1;
          continue;
        }
        if (jvs_escape) {
          incomingByte++;
          jvs_escape = 0;
        }
        if (jvs_phase == 1) {
          jvs_dest = incomingByte;
          jvs_checksum = (jvs_checksum + jvs_dest) & 0xFF;
          jvs_phase = 2;
        } else if (jvs_phase == 2) {
          jvs_length = incomingByte;
          jvs_checksum = (jvs_checksum + jvs_length) & 0xFF;
          jvs_phase = 3;
        } else if (jvs_phase == 3) {
          jvs_read++;
          if (jvs_read < jvs_length) {
            jvs_msg[jvs_read-1] = incomingByte;
//            memset(jvs_msg, jvs_read-1, incomingByte);
            jvs_checksum = (jvs_checksum + incomingByte) & 0xFF;
          } else {
            if (jvs_checksum != incomingByte) {
              Serial.println("Checksum error");
              jvs_phase=0;
              break;
            } else {
              //Serial.println("OK to process");
              if (jvs_read > 0) JVS_process_packet(jvs_dest,jvs_read-1,jvs_msg);
              break;
            }
          }
        }
       } // -1
     } // while true  
  }
}

void JVS_process_packet(byte jvs_dest, byte jvs_length, byte jvs_msg[128]) {
/*
  Serial.print("JVS in processor. Dest: 0x");
  Serial.print(jvs_dest,HEX);
  Serial.print("  Size: 0x");
  Serial.print(jvs_length);
  Serial.println("");
*/
  
  if (jvs_dest == 0xFF && jvs_length == 0x02 && jvs_msg[0] == 0xF0) {
    // Bus reset
    io_board.boardAddress = 0;  // can't be 0 for JVS slave, so 0 means NG
    pinMode(SENSE_PIN, INPUT);  // Return SENSE line to floating state
    io_board.boardAddress = 0;
    Serial.println("Bus reset");
  } else if (jvs_dest == 0xFF && jvs_length == 0x02 && jvs_msg[0] == 0xF1) {
    // Slave set address 
    byte reply[] = {0x01,0x01};
    JVS_send_packet(0x00, 2, reply);
    pinMode(SENSE_PIN, OUTPUT);
    digitalWrite(SENSE_PIN,LOW);
    io_board.boardAddress = jvs_msg[1];

  } else if (io_board.boardAddress > 0) {
    if (jvs_dest == io_board.boardAddress || jvs_dest == 0xFF) {
      byte reply[128];
      byte rlen = 0;
      byte cmd_size = 0;
      reply[rlen++] = REPORT_SUCCESS;  // OK

      while (true) {
        if (jvs_msg[0] == CMD_COMMAND_VERSION) {
          cmd_size = 1;
          reply[rlen++] = REPORT_SUCCESS;
          reply[rlen++] = BOARD_CMD_VER;
        } else if (jvs_msg[0] == CMD_JVS_VERSION) {
          cmd_size = 1;
          reply[rlen++] = REPORT_SUCCESS;
          reply[rlen++] = BOARD_JVS_VER;
        } else if (jvs_msg[0] == CMD_COMMS_VERSION) {
          cmd_size = 1;
          reply[rlen++] = REPORT_SUCCESS;
          reply[rlen++] = BOARD_COM_VER;
        } else if (jvs_msg[0] == CMD_CAPABILITIES) {
          cmd_size = 1;
          reply[rlen++] = REPORT_SUCCESS; // CMD_CAPABILITIES OK
          reply[rlen++] = CAP_PLAYERS; // CAPS Player Block
          reply[rlen++] = BOARD_PLAYERS;
          reply[rlen++] = BOARD_BUTTONS;
          reply[rlen++] = 0x00; // CAPS player block padding

          if (BOARD_COINSLOTS > 0) {
            reply[rlen++] = CAP_COINS; 
            reply[rlen++] = BOARD_COINSLOTS;
            reply[rlen++] = 0x00; // CAPS coin block padding
            reply[rlen++] = 0x00; // CAPS coin block padding
          }
          if (BOARD_GUN_CHANNELS > 0) {
            reply[rlen++] = CAP_LIGHTGUN;
            reply[rlen++] = BOARD_GUN_RESBITS_X;
            reply[rlen++] = BOARD_GUN_RESBITS_Y;
            reply[rlen++] = BOARD_GUN_CHANNELS;
          }
          if (BOARD_GPO_BANKS > 0) {
             reply[rlen++] = CAP_GPO;
             reply[rlen++] = BOARD_GPO_BANKS;
             reply[rlen++] = 0x00;
             reply[rlen++] = 0x00;
          }
          if (BOARD_GPI_BANKS > 0) {
             reply[rlen++] = CAP_GPI;
             reply[rlen++] = BOARD_GPI_BANKS;
             reply[rlen++] = 0x00;
             reply[rlen++] = 0x00;
          } 
          if (BOARD_ANALOG_IN > 0) {
             reply[rlen++] = CAP_ANALOG_IN;
             reply[rlen++] = BOARD_ANALOG_IN;
             reply[rlen++] = BOARD_ANALOG_RESBITS;
             reply[rlen++] = 0x00;
          }
          reply[rlen++] = CAP_END;
        } else if (jvs_msg[0] == CMD_REQUEST_ID) {
          cmd_size=1;
          reply[rlen++] = REPORT_SUCCESS; // OK!
          memcpy(reply+rlen,BOARD_ID, sizeof(BOARD_ID));
          rlen +=  (sizeof(BOARD_ID) - 1); // -1 because of BOARD_ID is NULL term string, we do not need that NULL
          reply[rlen++] = 0x00;

        } else if (jvs_msg[0] == CMD_NAMCO_SPECIFIC) {
          if (jvs_msg[1] == 0x04) {
             cmd_size = 4;
             byte n1_reply[] = { 0x01, 0xFF, 0xFF, 0x01, 0x19, 0x99, 0x10, 0x26, 0x02, 0x18, 0x31, 0x55};
             memcpy(reply+rlen,n1_reply, sizeof(n1_reply));
             rlen +=  (sizeof(n1_reply));
          } else if (jvs_msg[1] == 0x18) {
             cmd_size = 6;
             byte n1_reply[] = { 0x01, 0x01 };
             memcpy(reply+rlen,n1_reply, sizeof(n1_reply));
             rlen +=  (sizeof(n1_reply));
          } else if (jvs_msg[1] == 0x40) {
             cmd_size = 4;
             byte n1_reply[] = { 0x01, 0x01 };
             memcpy(reply+rlen,n1_reply, sizeof(n1_reply));
             rlen +=  (sizeof(n1_reply));
          }
        } else if (jvs_msg[0] == CMD_READ_SWITCHES) {
          cmd_size = 3;
          // jvs_msg[1] says how many players to read
          // jvs_msg[2] says how many databanks per player to read
          
          reply[rlen++] = REPORT_SUCCESS; // SW block OK!
          reply[rlen++] = io_board.swSystem;

          for (int i = 0; i < BOARD_PLAYERS; i++) {
            for (int j = 0; j < BOARD_BUTBANKS; j++) {
                reply[rlen++] = io_board.swPlayerBank[i].swBank[j];
            }
          }
        } else if (jvs_msg[0] == CMD_READ_COINS) {
            cmd_size = 2;
          // jvs_msg[1] says how many coin slots
            reply[rlen++] = REPORT_SUCCESS;
            for (int i = 0; i < BOARD_COINSLOTS; i++) {
              reply[rlen++] = io_board.coinSlots[i].b1;
              reply[rlen++] = io_board.coinSlots[i].b2;
            }
        } else if (jvs_msg[0] == CMD_READ_LIGHTGUN) {
            cmd_size = 2;
          // jvs_msg[1] says how many guns 
            reply[rlen++] = REPORT_SUCCESS;
            for (int i = 0; i < BOARD_GUN_CHANNELS; i++) {
              int a_board_gun_posx = io_board.lightGuns[i].gun_pos_x << (0x10 - BOARD_GUN_RESBITS_X);
              int a_board_gun_posy = io_board.lightGuns[i].gun_pos_y << (0x10 - BOARD_GUN_RESBITS_Y);
              reply[rlen++] = (a_board_gun_posx >> 8) & 0xFF;
              reply[rlen++] = a_board_gun_posx & 0xFF;
              reply[rlen++] = (a_board_gun_posy >> 8) & 0xFF;
              reply[rlen++] = a_board_gun_posy & 0xFF;
            }            
        } else if (jvs_msg[0] == CMD_WRITE_GPO) {
            cmd_size = 3;
            reply[rlen++] = REPORT_SUCCESS;
        } else {
            Serial.println("Unknown JVS command");
            for(int i = 0; i < jvs_length; i++) {
            Serial.print(" 0x");
            Serial.print(jvs_msg[i],HEX);
            Serial.println("");
              
            Serial.println("--------------------");
      }
        }
        
        jvs_length = jvs_length - cmd_size;
        jvs_msg = jvs_msg + cmd_size;       
        if (jvs_length < 1) break;        
      } // while true
      /*
      Serial.println("Broke");
      for(int i = 0; i < rlen; i++) {
        Serial.print(" 0x");
        Serial.print(reply[i],HEX);
      }
      */
      JVS_send_packet(0x00, rlen, reply);
    } // our address or broadcast 0xFF
  }
}

void JVS_send_packet (byte jvs_dest, byte jvs_length, byte jvs_msg[128]) {
    byte jvs_checksum = 0;
    byte outBuffer[132];
    byte outBytes;
        
    outBuffer[0] = SYNC;
    outBuffer[1] = jvs_dest;
    jvs_checksum = (jvs_checksum + jvs_dest) & 0xFF;
    outBuffer[2] = jvs_length + 1;
    jvs_checksum = (jvs_checksum + jvs_length + 1) & 0xFF;
    outBytes = 3;
    
    for(int i = 0; i < jvs_length; i++)
    {
      if (jvs_msg[i] == SYNC || jvs_msg[i] == ESCAPE) {
        outBuffer[outBytes++] = ESCAPE;
        outBuffer[outBytes++] = jvs_msg[i] - 1;
      } else {
        outBuffer[outBytes++] = jvs_msg[i];
      }
      jvs_checksum = (jvs_checksum + jvs_msg[i]) & 0xFF;
    }
    if (jvs_checksum == SYNC || jvs_checksum == ESCAPE) {
        outBuffer[outBytes++] = ESCAPE;
        outBuffer[outBytes++] = jvs_checksum - 1;   
    } else {
        outBuffer[outBytes++] = jvs_checksum;
    }
    Serial1.write(outBuffer,outBytes);
/*
    Serial.print("Sent: ");
    for(int i = 0; i < jvs_length; i++) {
      Serial.print(" 0x");
      Serial.print(jvs_msg[i],HEX);
      
    }
    */
}
