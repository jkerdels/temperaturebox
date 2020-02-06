
#include <stdint.h>

#include <EEPROM.h>


// config

int DS18B20_PIN    = A0; // temperature sensor
int BUTTON_PIN     = A1;
int TM1637_DIO_PIN = A2; // DIO of LED-Display
int TM1637_CLK_PIN = A3; // CLK of LED-Display

const int TM1637_us = 10; // duration of half a clock cylce in microseconds
                          // used for two wire communication

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
// code to read DS18B20 temperature sensors

void DS18B20_setup() {
  pinMode(DS18B20_PIN,INPUT);
}

byte DS18B20_crc(byte crc, byte data) {
  for (byte bit = 0; bit < 8; ++bit) {
    byte i = ((data >> bit) & 1) ^ (crc & 1);
    crc >>= 1;
    crc ^= i << 2;
    crc ^= i << 3;
    crc |= i << 7;
  }
  return crc;
}

// sends a reset pulse and returns the number of presence pulses
byte DS18B20_reset() {
  byte result = 0;
  unsigned long time;
  
  pinMode(DS18B20_PIN,OUTPUT);
  digitalWrite(DS18B20_PIN,LOW);
  delayMicroseconds(500);
  pinMode(DS18B20_PIN,INPUT);

  time = micros();
  while ((micros() - time) < 500) {
    if (digitalRead(DS18B20_PIN) == HIGH) continue;
    result += 1;
    delayMicroseconds(10);
    while (digitalRead(DS18B20_PIN) == LOW);
    time = micros();
    delayMicroseconds(10);
  }  
  return result;
}

void DS18B20_master_write_0() {
  pinMode(DS18B20_PIN,OUTPUT);
  digitalWrite(DS18B20_PIN,LOW);
  delayMicroseconds(75);
  pinMode(DS18B20_PIN,INPUT);
  delayMicroseconds(5);
}

void DS18B20_master_write_1() {
  pinMode(DS18B20_PIN,OUTPUT);
  digitalWrite(DS18B20_PIN,LOW);
  delayMicroseconds(5);
  pinMode(DS18B20_PIN,INPUT);
  delayMicroseconds(75);
}

byte DS18B20_master_read() {
  byte result = 0;

  pinMode(DS18B20_PIN,OUTPUT);
  digitalWrite(DS18B20_PIN,LOW);
  delayMicroseconds(3);
  pinMode(DS18B20_PIN,INPUT);
  delayMicroseconds(2);

  result = digitalRead(DS18B20_PIN);

  delayMicroseconds(55);

  while (digitalRead(DS18B20_PIN) == LOW);

  delayMicroseconds(5);

  return result;
}

void DS18B20_write_byte(byte data) {
  for (byte bit = 0; bit < 8; ++bit) {
    if ((data >> bit) & 1) {
      DS18B20_master_write_1();
    } else {
      DS18B20_master_write_0();
    }
  }
}

byte DS18B20_read_byte() {
  byte result = 0;
  for (byte bit = 0; bit < 8; ++bit) {
    result |= (DS18B20_master_read() << bit);
  }
  return result;
}

void DS18B20_read_rom(byte *rom_id) {
  DS18B20_write_byte(0x33);
  for(byte i = 0; i < 8; ++i)
    rom_id[i] = DS18B20_read_byte();
}

void DS18B20_skip_rom() {
  DS18B20_write_byte(0xCC);
}

void DS18B20_match_rom(byte* rom_id) {
  DS18B20_write_byte(0x55);
  for (byte b = 0; b < 8; ++b) {
    DS18B20_write_byte(rom_id[b]);
  }
}

void DS18B20_convert_t(bool blocking = true) {
  DS18B20_write_byte(0x44);
  if (!blocking) return;

  while (DS18B20_master_read() == 0);
}

void DS18B20_write_scratchpad(byte TH, byte TL, byte conf) {
  DS18B20_write_byte(0x4E);
  DS18B20_write_byte(TH);
  DS18B20_write_byte(TL);
  DS18B20_write_byte(conf);
}

void DS18B20_read_scratchpad(byte* output, byte count = 9) {
  if (count > 9) count = 9;

  DS18B20_write_byte(0xBE);  
  for (byte b = 0; b < count; ++b) {
    output[b] = DS18B20_read_byte();
  }

  if (count < 9) {
    DS18B20_reset();
  }
}

void DS18B20_copy_scratchpad() {
  DS18B20_write_byte(0x48);
}

void DS18B20_recall_e2() {
  DS18B20_write_byte(0xB8);
}

byte DS18B20_read_power_supply() {
  DS18B20_write_byte(0xB4);
  return DS18B20_master_read();
}

uint16_t DS18B20_read_temp() {
  uint16_t result = 0;
  DS18B20_read_scratchpad((byte*)&result,2);
  return result;
}

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------
// code to control a 4 Digit 7 Segment LED Display

void TM1637_setup() {
  pinMode(TM1637_DIO_PIN,OUTPUT);
  pinMode(TM1637_CLK_PIN,OUTPUT);

  digitalWrite(TM1637_CLK_PIN,HIGH);
  digitalWrite(TM1637_DIO_PIN,HIGH);
}

void TM1637_start_input() {
  digitalWrite(TM1637_CLK_PIN,HIGH);
  digitalWrite(TM1637_DIO_PIN,HIGH);

  delayMicroseconds(100);

  digitalWrite(TM1637_DIO_PIN,LOW);
  delayMicroseconds(TM1637_us);
  digitalWrite(TM1637_CLK_PIN,LOW);
}

void TM1637_stop_input() {  
  digitalWrite(TM1637_CLK_PIN,LOW);
  digitalWrite(TM1637_DIO_PIN,LOW);
  delayMicroseconds(TM1637_us);
  digitalWrite(TM1637_CLK_PIN,HIGH);
  delayMicroseconds(TM1637_us);
  digitalWrite(TM1637_DIO_PIN,HIGH);
}

void TM1637_send_data(const byte data) {

  for (byte bit = 0; bit < 8; ++bit) {
    digitalWrite(TM1637_CLK_PIN,LOW);
    delayMicroseconds(TM1637_us/2);
    digitalWrite(TM1637_DIO_PIN,(data >> bit) & 1);
    delayMicroseconds(TM1637_us/2);
    digitalWrite(TM1637_CLK_PIN,HIGH);
    delayMicroseconds(TM1637_us);
  }
  digitalWrite(TM1637_CLK_PIN,LOW);
  digitalWrite(TM1637_DIO_PIN,LOW);
  delayMicroseconds(TM1637_us);
  digitalWrite(TM1637_CLK_PIN,HIGH);
  delayMicroseconds(TM1637_us);
  
}

void TM1637_display_off() {
  TM1637_start_input();
  TM1637_send_data(0x80);
  TM1637_stop_input();
}

void TM1637_display_on(byte brightness = 7) {
  if (brightness > 7) {
    brightness = 7;
  }
  TM1637_start_input();
  TM1637_send_data(0x88 | brightness);
  TM1637_stop_input();  
}

void TM1637_write(byte address, byte data) {
  TM1637_start_input();
  TM1637_send_data(address);
  TM1637_send_data(data);
  TM1637_stop_input(); 
}

void TM1637_write(byte address, byte data1, byte data2) {
  TM1637_start_input();
  TM1637_send_data(address);
  TM1637_send_data(data1);
  TM1637_send_data(data2);
  TM1637_stop_input(); 
}

void TM1637_write(byte address, byte data1, byte data2, byte data3) {
  TM1637_start_input();
  TM1637_send_data(address);
  TM1637_send_data(data1);
  TM1637_send_data(data2);
  TM1637_send_data(data3);
  TM1637_stop_input(); 
}

void TM1637_write(byte address, byte data1, byte data2, byte data3, byte data4) {
  TM1637_start_input();
  TM1637_send_data(address);
  TM1637_send_data(data1);
  TM1637_send_data(data2);
  TM1637_send_data(data3);
  TM1637_send_data(data4);
  TM1637_stop_input(); 
}

void TM1637_begin_write() {
  TM1637_start_input();
  TM1637_send_data(0x40);
  TM1637_stop_input();   
}

byte AZ4D[] = {
  B00111111, // segments for '0'
  B00000110, // segments for '1'
  B01011011, // segments for '2'
  B01001111, // segments for '3'
  B01100110, // segments for '4'
  B01101101, // segments for '5'
  B01111101, // segments for '6'
  B00000111, // segments for '7'
  B01111111, // segments for '8'
  B01101111, // segments for '9'
  B01000000, // segments for '-'
  B10000000, // segments for ':'
  B01111001  // segments for 'E'
};

//------------------------------------------------------------------------------------------------
//------------------------------------------------------------------------------------------------


byte current_slot;

byte current_mode;

byte rom_IDs[80];

byte temp[9];

void setup() {
  // put your setup code here, to run once:
  DS18B20_setup();
  TM1637_setup();
  pinMode(BUTTON_PIN,INPUT_PULLUP);

  TM1637_begin_write();
  TM1637_write(0xC0, 0,0,0,0);
  TM1637_display_on(3);  

  current_slot = 1;
  current_mode = 0;

  byte magic = EEPROM.read(80);

  if (magic != 123) {
    // first time we started, clear EEPROM
    for (byte i = 0; i < 80; ++i)
      EEPROM.write(i, 0);
    EEPROM.write(80,123);
  }

  // get rom_IDs from EEPROM
  for (byte i = 0; i < 80; ++i)
    rom_IDs[i] = EEPROM.read(i);
}


void loop() {
  // display current slot
  TM1637_begin_write();
  TM1637_write(0xC0, AZ4D[current_slot]);
  TM1637_display_on(5);

  switch(current_mode) {
    // normal operation
    case 0 : {      
      if (rom_IDs[current_slot*8] == 0) {
        // this is an empty slot
        TM1637_begin_write();
        TM1637_write(0xC1, 0, AZ4D[10], AZ4D[10]);  
        TM1637_display_on(5);
      } else {
        DS18B20_reset();
        DS18B20_match_rom(rom_IDs + (current_slot*8));
        DS18B20_convert_t();

        DS18B20_reset();
        DS18B20_match_rom(rom_IDs + (current_slot*8));
        DS18B20_read_scratchpad(temp);

        byte crc = 0;
        for (byte i = 0; i < 9; ++i)
          crc = DS18B20_crc(crc,temp[i]);

        if (crc != 0) {
          // transmission error
          TM1637_begin_write();
          TM1637_write(0xC1, 0, 0, AZ4D[12]);
          TM1637_display_on(5);
        } else {
          // get temperature and display it
          uint8_t t_val = temp[0] >> 4 | temp[1] << 4;
          TM1637_begin_write();
          if (t_val >= 100) {
            TM1637_write(0xC1, AZ4D[(t_val / 100) % 10], AZ4D[(t_val / 10) % 10], AZ4D[t_val % 10]);
          } else {
            TM1637_write(0xC1, 0, AZ4D[(t_val / 10) % 10], AZ4D[t_val % 10]);
          }
          TM1637_display_on(5);          
        }
      }      
    } break;

    // learning of a new sensor
    case 1 : {
      TM1637_begin_write();
      TM1637_write(0xC1, AZ4D[11], 0, 0);
      TM1637_display_on(5);
      byte nr_of_sensors = DS18B20_reset();
      if (nr_of_sensors == 1) {
        DS18B20_read_rom(rom_IDs + (current_slot*8));

        byte crc = 0;
        for (byte i = 0; i < 8; ++i)
          crc = DS18B20_crc(crc,rom_IDs[i + (current_slot*8)]);

        if (crc == 0) {
          // store new rom_ID in EEPROM
          for (byte i = 0; i < 8; ++i)
            EEPROM.update(current_slot*8+i, rom_IDs[i + (current_slot*8)]);

          // configure new sensor
          DS18B20_reset();
          DS18B20_skip_rom();
          DS18B20_write_scratchpad(0,0,B00011111);

          DS18B20_reset();
          DS18B20_skip_rom();
          DS18B20_copy_scratchpad();

          // switch to normal mode
          current_mode = 0;
        } else {
          for (byte i = 0; i < 8; ++i)
            rom_IDs[i + (current_slot*8)] = 0;
        }
      }
    } break;    
  }

  // handle button presses
  unsigned long time = millis();
  while (millis() - time < 100) {
    if (digitalRead(BUTTON_PIN) == LOW) {
      delay(20);
      bool long_down = false;
      time = millis();
      while (digitalRead(BUTTON_PIN) == LOW) {
        if (millis() - time > 2000) {
          current_mode = (current_mode + 1) & 1;
          long_down = true;
          break;
        }
      }
      if ((long_down == false) && (current_mode == 0)) {
        current_slot = (current_slot + 1) % 10;
      }
      break;
    }  
  }
}
