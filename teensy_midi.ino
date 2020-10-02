#include <Adafruit_NeoPixel.h>
#include <MIDI.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <extEEPROM.h>

#define n_messages 8
#define n_presets 8
#define n_banks 12
#define n_values 4
#define save_button 2
#define prev_button 3
#define next_button 4
#define exit_button 5
#define OMNIPORT_NUMBER 2
#define EXP_TYPE 1
#define FXD1_SWITCH_TYPE 2
#define FXD2_SWITCH_TYPE 3


// Settings
boolean sendMIDIMonitor = false;
byte portType[OMNIPORT_NUMBER];
byte ringBright;
byte ringDim;
boolean editMode = false;
unsigned int debounceTime;
unsigned int longPressTime;
unsigned int notificationTime;
unsigned long pressedTime  = 0;
short expDown[OMNIPORT_NUMBER];
short expUp[OMNIPORT_NUMBER];
char presetsName[16] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'};


// EEPROM
const int eepromAddress = 0x50;
extEEPROM myEEPROM(kbits_256, 1, 64);
byte i2cStat;


// MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);


// Expression Pedals
const short POT_THRESHOLD = 7;        // Threshold amount to guard against false values

// LCD
LiquidCrystal_I2C lcd(0x3F, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE); 

// Neopixels
Adafruit_NeoPixel pixels = Adafruit_NeoPixel(64, 12, NEO_GRB + NEO_KHZ800);

// Colors
struct Color
{
  byte r;
  byte g;
  byte b;
};

struct Color colors[] = {
  {0,0,0},        // 0 - Black
  {255,0,0},      // 1 - Red
  {255,165,0},    // 2 - Orange
  {255,255,0},    // 3 - Yellow
  {0,255,0},      // 4 - Green
  {0,0,255},      // 5 - Blue
  {0,191,255},    // 6 - Cyan
  {128,0,128},    // 7 - Purple
  {255,255,255},  // 8 - White
  {255,105,180},  // 9 - Pink
  {0,139,139},    // 10 - Turquoise
  {173,255,47}    // 11 - Lime
};

// Data
byte bankNumber;
byte pageNumber;
byte buttonNumber;
byte presetBankNumber;
byte presetPageNumber;
byte presetButtonNumber;

// Messages
struct Msg
  {
    byte action;
    byte type;
    byte pos;
    byte value[n_values];
  };
struct ExpMsg
  {
    byte type;
    byte value[n_values];
  };
struct Preset
  {
    byte presetConf;                  // Bit 6 - lpToggleMode, Bit 5 - spToggleMode, Bit 4-0 Preset Type
    char longName[25] = "EMPTY";
    char pShortName[9] = "EMPTY";
    char pToggleName[9] = "EMPTY";
    char lpShortName[9] = "EMPTY";
    char lpToggleName[9] = "EMPTY";
    byte color;                       // Bit 6 - Color type (Off, Dimmer), Bit 5-0 color
    struct Msg message[n_messages];
  };
struct Page
  {
    struct Preset preset[n_presets];
  };
struct OmniPort
  {
    char shortName[9] = "EXP";
    struct ExpMsg message[n_messages];
  };
struct Bank
  {
    char bankName[25];
    struct Page page[2];
    struct OmniPort port[2];
  };
struct Data
  {
    struct Bank bank[n_banks];
  } data;

// Single & Long Press Positions
struct PosPreset
  {
    byte posSingle = 0;
    byte posLong = 0;
  };
struct PosPage
  {
    struct PosPreset preset[n_presets];
  };
struct PosBank
  {
    struct PosPage page[2];
  };
struct PosData
  {
    struct PosBank bank[n_banks];
  } posData;

void lcdBankButton(boolean presetRefresh = true);

void setup() {
  for(int i=2; i<=9; i++){
    pinMode(i, INPUT);
  }
  
  MIDI.begin();

  lcd.begin(16,2);//Defining 16 columns and 2 rows of lcd display
  lcd.backlight();//To Power ON the back light

  i2cStat = myEEPROM.begin(extEEPROM::twiClock400kHz);
  if ( i2cStat != 0 ) {
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("EEPROM r error"));
    while (true) {}
  }

  // EEPROM
  debounceTime = EEPROM[1] * 10;
  longPressTime =  EEPROM[2] * 10;
  notificationTime =  EEPROM[3] * 10;
  ringBright = EEPROM[4];
  ringDim = EEPROM[5];
  for(int i=0; i<OMNIPORT_NUMBER; i++){
    EEPROM.get((6+i),portType[i]);
  }
  for(int i=0; i<OMNIPORT_NUMBER; i++){
    EEPROM.get(10+(4*i),expDown[i]);
    EEPROM.get((10+(4*i))+2,expUp[i]);
  }
  
  bankNumber = EEPROM[0];
  pageNumber = 1;
  buttonNumber = 1;
  presetBankNumber = 0;
  presetPageNumber = 0;
  presetButtonNumber = 0;

  myEEPROM.read(0,(byte*)&data, sizeof(data));
  //myEEPROM.write(0,(byte*)&data, sizeof(data));
  usbMIDI.setHandleSystemExclusive(OnSysEx);

  pixels.begin();
  pixels.setBrightness(32);
  drawColors();
  lcdBankButton();
}

void loop() {
  usbMIDI.read();
  checkButtons();
  checkOmniPorts();
}

void drawColors() {
  for(int i=0; i<8; i++){
    byte r;
    byte g;
    byte b;
    byte presetType = 0b00011111&(data.bank[bankNumber-1].page[pageNumber-1].preset[i].presetConf);
    int colorNumber = 0b00111111&(data.bank[bankNumber-1].page[pageNumber-1].preset[i].color);
    byte colorType = bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[i].color, 6);
    r = colors[colorNumber].r;
    g = colors[colorNumber].g;
    b = colors[colorNumber].b;
    
    if (presetType == 0) {
      if (bankNumber == presetBankNumber && pageNumber == presetPageNumber && (i+1) == presetButtonNumber) {
        for(int j=0; j<8; j++){
          pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
        }
      } else {
        if (colorType == 0) {
          for(int j=0; j<8; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
          }
        } else {
          for(int j=0; j<8; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
          }
        }
      }
    } else if (presetType == 1){
      if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingle == 0) {
        if (colorType == 0) {
          for(int j=0; j<8; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
          }
        } else {
          for(int j=0; j<8; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
          }
        }
      } else {
        for(int j=0; j<8; j++){
          pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
        }
      }
    }
  }
  pixels.show();
}

void checkOmniPorts() {
  for(int i=0; i<OMNIPORT_NUMBER; i++){
    if (portType[i] == EXP_TYPE) {
      checkExp(i);
    }
  }
}

void checkExp(byte pedalNumber) {
  static short s_nLastPotValue[OMNIPORT_NUMBER];
  static short s_nLastMappedValue[OMNIPORT_NUMBER];

  short nCurrentPotValue = analogRead(pedalNumber);
  if(abs(nCurrentPotValue - s_nLastPotValue[pedalNumber]) < POT_THRESHOLD)
      return;
  s_nLastPotValue[pedalNumber] = nCurrentPotValue;

  short nMappedValue = map(nCurrentPotValue, expDown[pedalNumber], expUp[pedalNumber], 0, 127); // Map the value to 0-127
  
  if(nMappedValue < 0 || nMappedValue > 127 || nMappedValue == s_nLastMappedValue[pedalNumber])
      return;
  s_nLastMappedValue[pedalNumber] = nMappedValue;
  
  if (editMode) {
    sendUSBExpData(pedalNumber);
  }
  
  short percentValue = map(nMappedValue, 0, 127, 0, 100);
  lcd.setCursor(15,1);
  lcd.print(F("%"));
  if (percentValue < 10) {
    lcd.setCursor(12,1);
    lcd.print(F("  "));
    lcd.setCursor(14,1);
    lcd.print(percentValue);
  } else if (percentValue >= 10 && percentValue < 100) {
    lcd.setCursor(12,1);
    lcd.print(F(" "));
    lcd.setCursor(13,1);
    lcd.print(percentValue);
  } else {
    lcd.setCursor(12,1);
    lcd.print(percentValue);
  }
  // Send MIDI Msg
  for (byte i=0; i < n_messages; i++) {
    switch (data.bank[bankNumber-1].port[pedalNumber].message[i].type) {
      // Expression CC
      case 1:
        {
          byte minValue = data.bank[bankNumber-1].port[pedalNumber].message[i].value[1];
          byte maxValue = data.bank[bankNumber-1].port[pedalNumber].message[i].value[2];
          byte mapValue = map(nMappedValue, 0, 127, minValue, maxValue);
          MIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], mapValue, data.bank[bankNumber-1].port[pedalNumber].message[i].value[3]);
          if (sendMIDIMonitor && editMode) {
            usbMIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], mapValue, data.bank[bankNumber-1].port[pedalNumber].message[i].value[3]);
          }
        }
        break;
      // CC Down
      case 2:
        if (nMappedValue == 0) {
          MIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], data.bank[bankNumber-1].port[pedalNumber].message[i].value[1], data.bank[bankNumber-1].port[pedalNumber].message[i].value[2]);
          if (sendMIDIMonitor && editMode) {
            usbMIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], data.bank[bankNumber-1].port[pedalNumber].message[i].value[1], data.bank[bankNumber-1].port[pedalNumber].message[i].value[2]);
          }
        }
        break;
      // CC Up
      case 3:
        if (nMappedValue == 127) {
          MIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], data.bank[bankNumber-1].port[pedalNumber].message[i].value[1], data.bank[bankNumber-1].port[pedalNumber].message[i].value[2]);
          if (sendMIDIMonitor && editMode) {
            usbMIDI.sendControlChange(data.bank[bankNumber-1].port[pedalNumber].message[i].value[0], data.bank[bankNumber-1].port[pedalNumber].message[i].value[1], data.bank[bankNumber-1].port[pedalNumber].message[i].value[2]);
          }
        }
        break;
    }
  }
  //lcd.setCursor(9,1);
  //lcd.print(nCurrentPotValue);
  //MidiVolume(MIDI_CHANNEL, nMappedValue);
}

void checkButtons() {
  for(byte i=2; i<=(n_presets+1); i++){
    if(digitalRead(i)== HIGH){
      pressedTime = millis();
      delay(debounceTime);
      if(digitalRead(i)== HIGH){
        // A+B check (Bank Down)
        if (digitalRead(2)== HIGH && digitalRead(3)== HIGH) {
          pageNumber = 1;
          bankDown();
          drawColors();
          checkMenuButtonRelease();
        // B+C check (Toggle Page)
        } else if (digitalRead(3)== HIGH && digitalRead(4)== HIGH) {
          togglePag();
          drawColors();
          checkMenuButtonRelease();
        // C+D check (Bank Up)
        } else if (digitalRead(4)== HIGH && digitalRead(5)== HIGH) {
          pageNumber = 1;
          bankUp();
          drawColors();
          checkMenuButtonRelease();
        // D+E check (Web Editor Mode)
        } else if (digitalRead(5)== HIGH && digitalRead(6)== HIGH) {
          if (editMode) {
            editMode = false;
            lcd.clear();
            lcd.setCursor(1,0);
            lcd.print(F("EDIT MODE OFF"));
            delay(800);
            lcdBankButton();
          } else {
            editMode = true;
            lcd.clear();
            lcd.setCursor(1,0);
            lcd.print(F("EDIT MODE ON"));
            delay(800);
            lcdBankButton();
          }
          checkMenuButtonRelease();
        // E+H check (Edit Menu)
        } else if (digitalRead(6)== HIGH && digitalRead(9)== HIGH) {
          if (!editMode) {
            pixels.clear();
            pixels.show();
            showConfMenu(1);
            checkMenuButtonRelease();
            confMenu();
            lcdBankButton();
            drawColors();
            checkMenuButtonRelease();
          } else {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("Exit Edit Mode"));
            lcd.setCursor(0,1);
            lcd.print(F("First"));
            delay(800);
            lcdBankButton();
          }
        } else {
          btnPressed(i);
          // Trigger Release All Action
          checkMsg(9);
          drawColors();
          lcdBankButton();
        }
      }
    }
  }
}

void btnPressed(byte i) {
  buttonNumber = i-1;
  
  if (editMode) {
    sendUSBPresetData();
  }
  // Trigger Press Action
  checkMsg(1);
  
  while (true) {
    // Long Press
    if ((millis() - pressedTime) > longPressTime) {
      // Trigger Long Press Action
      checkMsg(3);

      while (true) {
        if (digitalRead(i)== LOW) {
          // Trigger Long Press Release Action
          checkMsg(4);
          // Change Toggle
          if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 6) == 1) {
            if (posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong == 0) {
              posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong = 1;
            } else {
              posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong = 0;
            }
          }
          return;
        }
      }
    }
    // Release
    if (digitalRead(i)== LOW) {
      if ((0b00011111&(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf)) == 0) {
        presetBankNumber = bankNumber;
        presetPageNumber = pageNumber;
        presetButtonNumber = buttonNumber;
      }
      // Trigger Release Action
      checkMsg(2);
      // Change Toggle
      if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 5) == 1) {
        if (posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle == 0) {
          posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle = 1;
        } else {
          posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle = 0;
        }
      }
      return;
    }
  }
}

void checkMsg(byte action) {
  for (byte i=0; i < n_messages; i++) {
    if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].action == action) {
      switch (action) {
        case 1:
        case 2:
          if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle ||
              data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == 2) {
            sendMIDIMessage(i);
          }
          break;
        case 3:
        case 4:
          if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong ||
              data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == 2) {
            sendMIDIMessage(i);
          }
          break;
        case 9:
          sendMIDIMessage(i);
          break;
      }
    }
  }
}

void sendMIDIMessage(byte i) {
  switch (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].type) {
    // Program Change
    case 1:
      MIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      if (sendMIDIMonitor && editMode) {
        usbMIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      }
      break;
    // Control Change
    case 2:
      MIDI.sendControlChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      if (sendMIDIMonitor && editMode) {
        usbMIDI.sendControlChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      }
      break;
    // Bank Up
    case 10:
      pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]) + 1;
      bankUp();
      break;
    // Bank Down
    case 11:
      pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]) + 1;
      bankDown();
      break;
    // Bank Jump
    case 13:
      bankNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]) + 1;
      break;
    // Toggle Page
    case 14:
      togglePag();
      break;
    // Set Toggle Single
    case 15:
    {
      byte actionToggle = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      byte num1 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      byte num2 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      byte num3 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[3]);
      
      for(int i=0; i<7; i++){
        if (bitValue(num1, i) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[bankNumber-1].page[0].preset[i].posSingle = 1;
          } else if (actionToggle == 2) {
            posData.bank[bankNumber-1].page[0].preset[i].posSingle = 0;
          }
        } else if ((bitValue(num1, i) == 0) && (actionToggle == 0)) {
          posData.bank[bankNumber-1].page[0].preset[i].posSingle = 0;
        }
      }
      for(int i=7; i<14; i++){
        if (bitValue(num2, (i-7)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            if (i < n_presets) {
              posData.bank[bankNumber-1].page[0].preset[i].posSingle = 1;
            } else {
              posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posSingle = 1;
            }
          } else if (actionToggle == 2) {
            if (i < n_presets) {
              posData.bank[bankNumber-1].page[0].preset[i].posSingle = 0;
            } else {
              posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posSingle = 0;
            }
          }
        } else if ((bitValue(num2, (i-7)) == 0) && (actionToggle == 0)) {
          if (i < n_presets) {
            posData.bank[bankNumber-1].page[0].preset[i].posSingle = 0;
          } else {
            posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posSingle = 0;
          }
        }
      }
      for(int i=14; i<16; i++){
        if (bitValue(num3, (i-14)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[bankNumber-1].page[1].preset[i-n_presets].posSingle = 1;
          } else if (actionToggle == 2) {
            posData.bank[bankNumber-1].page[1].preset[i-n_presets].posSingle = 0;
          }
        } else if ((bitValue(num3, (i-14)) == 0) && (actionToggle == 0)) {
          posData.bank[bankNumber-1].page[1].preset[i-n_presets].posSingle = 0;
        }
      }
      break;
    }
    // Set Toggle Long
    case 16:
    {
      byte actionToggle = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      byte num1 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      byte num2 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      byte num3 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[3]);
      
      for(int i=0; i<7; i++){
        if (bitValue(num1, i) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[bankNumber-1].page[0].preset[i].posLong = 1;
          } else if (actionToggle == 2) {
            posData.bank[bankNumber-1].page[0].preset[i].posLong = 0;
          }
        } else if ((bitValue(num1, i) == 0) && (actionToggle == 0)) {
          posData.bank[bankNumber-1].page[0].preset[i].posLong = 0;
        }
      }
      for(int i=7; i<14; i++){
        if (bitValue(num2, (i-7)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            if (i < n_presets) {
              posData.bank[bankNumber-1].page[0].preset[i].posLong = 1;
            } else {
              posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posLong = 1;
            }
          } else if (actionToggle == 2) {
            if (i < n_presets) {
              posData.bank[bankNumber-1].page[0].preset[i].posLong = 0;
            } else {
              posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posLong = 0;
            }
          }
        } else if ((bitValue(num2, (i-7)) == 0) && (actionToggle == 0)) {
          if (i < n_presets) {
            posData.bank[bankNumber-1].page[0].preset[i].posLong = 0;
          } else {
            posData.bank[bankNumber-1].page[1].preset[(i-n_presets)].posLong = 0;
          }
        }
      }
      for(int i=14; i<16; i++){
        if (bitValue(num3, (i-14)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[bankNumber-1].page[1].preset[i-n_presets].posLong = 1;
          } else if (actionToggle == 2) {
            posData.bank[bankNumber-1].page[1].preset[i-n_presets].posLong = 0;
          }
        } else if ((bitValue(num3, (i-14)) == 0) && (actionToggle == 0)) {
          posData.bank[bankNumber-1].page[1].preset[i-n_presets].posLong = 0;
        }
      }
      break;
    }
    // Delay
    case 23:
      delay((data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0])*10);
      break;
  }
}

void sendUSBPresetData() {
  byte dataStart[] = { 0xF0, 0x01 };
  byte dataEnd[] = { 0xF7 };
  usbMIDI.sendSysEx(2, dataStart, true);
  usbMIDI.sendSysEx(sizeof(bankNumber), (byte*)&bankNumber, true);
  usbMIDI.sendSysEx(sizeof(pageNumber), (byte*)&pageNumber, true);
  usbMIDI.sendSysEx(sizeof(buttonNumber), (byte*)&buttonNumber, true);
  usbMIDI.sendSysEx(sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]), (byte*)&data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1], true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendUSBBankData() {
  byte dataStart[] = { 0xF0, 0x02 };
  byte dataEnd[] = { 0xF7 };
  usbMIDI.sendSysEx(2, dataStart, true);
  usbMIDI.sendSysEx(sizeof(data.bank[bankNumber-1].bankName), (byte*)&data.bank[bankNumber-1].bankName, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendSettingsData() {
  byte dataStart[] = { 0xF0, 0x03 };
  byte eepromData[] = { EEPROM[1], EEPROM[2], EEPROM[3], EEPROM[4], EEPROM[5], EEPROM[6], EEPROM[7] };
  byte dataEnd[] = { 0xF7 };
  usbMIDI.sendSysEx(2, dataStart, true);
  usbMIDI.sendSysEx(sizeof(eepromData), eepromData, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendUSBExpData(byte pedalNumber) {
  byte dataStart[] = { 0xF0, 0x08 };
  byte dataEnd[] = { 0xF7 };
  usbMIDI.sendSysEx(2, dataStart, true);
  usbMIDI.sendSysEx(sizeof(bankNumber), (byte*)&bankNumber, true);
  usbMIDI.sendSysEx(sizeof(pedalNumber), (byte*)&pedalNumber, true);
  usbMIDI.sendSysEx(sizeof(data.bank[bankNumber-1].port[pedalNumber]), (byte*)&data.bank[bankNumber-1].port[pedalNumber], true);  
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void OnSysEx(byte* readData, unsigned sizeofsysex) {
  if (editMode) {
    sendMIDIMonitor = false;
    unsigned sizeOfData = 0;
    int bankOffset = (bankNumber-1)*(sizeof(data.bank[bankNumber-1]));
    int presetOffset = (sizeof(data.bank[bankNumber-1].bankName)) + ((buttonNumber-1)*(sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1])));
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print(F("Communicating..."));
  
    switch (readData[1]) {
      // Save Preset Data
      case 1:
        sizeOfData = sizeof(struct Preset) + 6;
        if (sizeofsysex == sizeOfData) {
          bankNumber = readData[2];
          pageNumber = readData[3];
          buttonNumber = readData[4];
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1], &readData[5], sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]));
      
          // Guardado en EEPROM
          i2cStat = myEEPROM.write((bankOffset + presetOffset),(byte*)&readData[5], sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]));
          if ( i2cStat != 0 ) {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("EEPROM w error  "));
            while (true) {}
          }
          lcd.setCursor(0,0);
          lcd.print(F("Saved Preset    "));
          delay(500);
        }
        break;
        
      // Save Bank Data
      case 2:
        sizeOfData = sizeof(data.bank[bankNumber-1].bankName) + 4;
        if (sizeofsysex == sizeOfData) {
          bankNumber = readData[2];
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].bankName, &readData[3], sizeof(data.bank[bankNumber-1].bankName));
      
          // Guardado en EEPROM
          i2cStat = myEEPROM.write(bankOffset,(byte*)&readData[3], sizeof(data.bank[bankNumber-1].bankName));
          if ( i2cStat != 0 ) {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("EEPROM w error  "));
            while (true) {}
          }
          lcd.setCursor(0,0);
          lcd.print(F("Saved Bank      "));
          delay(500);
        }
        break;
        
      // Preset Data Request
      case 3:
        sizeOfData = 4;
        if (sizeofsysex == sizeOfData) {
          sendUSBPresetData();
        }
        break;
      // Bank Data Request
      case 4:
        sizeOfData = 4;
        if (sizeofsysex == sizeOfData) {
          sendUSBBankData();
        }
        break;
      // Settings Data Request
      case 5:
        sizeOfData = 4;
        if (sizeofsysex == sizeOfData) {
          sendSettingsData();
        }
        break;
      // Save Settings Data
      case 6:
        sizeOfData = 10;
        if (sizeofsysex == sizeOfData) {
          EEPROM.update(1, readData[2]);
          debounceTime = readData[2] * 10;
          EEPROM.update(2, readData[3]);
          longPressTime = readData[3] * 10;
          EEPROM.update(3, readData[4]);
          notificationTime = readData[4] * 10;
          EEPROM.update(4, readData[5]);
          ringBright =  readData[5];
          EEPROM.update(5, readData[6]);
          ringDim =  readData[6];
          EEPROM.update(6, readData[7]);
          portType[0] =  readData[7];
          EEPROM.update(7, readData[8]);
          portType[1] =  readData[8];
          
          lcd.setCursor(0,0);
          lcd.print(F("Saved Settings  "));
          delay(500);
        }
        break;
      // MIDI Monitor
      case 7:
        sendMIDIMonitor = true;
        break;
       // Save Exp Data
      case 8:
        sizeOfData = sizeof(struct OmniPort) + 5;
        if (sizeofsysex == sizeOfData) {
          bankNumber = readData[2];
          byte expNumber = readData[3];
          int expOffset = (sizeof(data.bank[bankNumber-1].bankName)) + (2*(sizeof(data.bank[bankNumber-1].page[0]))) + ((expNumber)*(sizeof(data.bank[bankNumber-1].port[expNumber])));
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].port[expNumber], &readData[4], sizeof(data.bank[bankNumber-1].port[expNumber]));
          // Guardado en EEPROM
          i2cStat = myEEPROM.write((bankOffset + expOffset),(byte*)&readData[4], sizeof(data.bank[bankNumber-1].port[expNumber]));
          if ( i2cStat != 0 ) {
            lcd.clear();
            lcd.setCursor(0,0);
            lcd.print(F("EEPROM w error  "));
            while (true) {}
          }
          lcd.setCursor(0,0);
          lcd.print(F("Saved Expression"));
          delay(500);
        }
        break;
    }
    delay(300);
    lcdBankButton();
  } else {
    if (readData[1] == 8) {
      editMode = true;
      lcd.clear();
      lcd.setCursor(1,0);
      lcd.print(F("EDIT MODE ON"));
      sendUSBPresetData();
      delay(800);
      lcdBankButton();
    }
  }
}

void lcdBankButton(boolean presetRefresh) {
  byte bankLCDOffset = 0;
  if (presetRefresh) {
    lcd.clear();
  }
  lcd.setCursor(0,0);
  lcd.print(F("B                   "));
  lcd.setCursor(1,0);
  lcd.print(bankNumber, DEC);
  if (bankNumber >= 10) {
    bankLCDOffset = 1;
  }

  lcd.setCursor(3+bankLCDOffset,0);
  lcd.print(F("P"));
  lcd.setCursor(4+bankLCDOffset,0);
  lcd.print(pageNumber, DEC);
  lcd.setCursor(5+bankLCDOffset,0);
  lcd.print(F(": "));
  lcd.setCursor(7+bankLCDOffset,0);
  lcd.print(F(data.bank[bankNumber-1].bankName));
  
  if (presetRefresh) {
    lcd.setCursor(0,1);
    lcd.print(F("PST"));
    lcd.setCursor(4,1);
    if (pageNumber == 1) {
      lcd.print(presetsName[buttonNumber-1]);
    } else {
      lcd.print(presetsName[(buttonNumber-1)+n_presets]);
    }
  }
  
}

byte bitValue(byte num, byte pos) {
  return ((num >> pos)&0x01);
}

void togglePag() {
  if (pageNumber == 1) {
    pageNumber = 2;
  } else {
    pageNumber = 1;
  }
  lcdBankButton(false);
}

void bankDown() {
  if (bankNumber == 1) {
    bankNumber = n_banks;
  } else {
    bankNumber--;
  }
  lcdBankButton(false);
}

void bankUp() {
  if (bankNumber == n_banks) {
    bankNumber = 1;
  } else {
    bankNumber++;
  }
  lcdBankButton(false);
}

bool checkMenuButton(byte i) {
  if(digitalRead(i)== HIGH){
    delay(20);
    if(digitalRead(i)== HIGH){
      return true;
    }
  }
  return false;
}

void showConfMenu(byte page) {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(F("Global Conf Menu"));
}

void checkMenuButtonRelease() {
  boolean releaseAll = false;
  while (!releaseAll) {
    releaseAll = true;
    for(int i=2; i<=9; i++){
      if(digitalRead(i)== HIGH){
        releaseAll = false;
      }
    }
  }
  delay(20);
}

void confMenu() {
  byte page = 1;
  while (true) {
    // Button A
    if(checkMenuButton(2)){
      switch (page) {
        case 1:
          confMenuDebounceTime();
          break;
        case 2:
          confMenuRingBright();
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button B
    } else if(checkMenuButton(3)){
      switch (page) {
        case 1:
          confMenuLongPressTime();
          break;
        case 2:
          confMenuRingDim();
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button C
    } else if(checkMenuButton(4)){
      switch (page) {
        case 1:
          confMenuNotificationTime();
          break;
        case 2:
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button E
    } else if(checkMenuButton(6)){
      switch (page) {
        case 1:
          confMenuOmniPort(0);
          break;
        case 2:
          confCalibrateExpDown(0);
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button F
    } else if(checkMenuButton(7)){
      switch (page) {
        case 1:
          confMenuOmniPort(1);
          break;
        case 2:
          confCalibrateExpDown(1);
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button I
    } else if(checkMenuButton(9)){
      switch (page) {
        case 1:
          page = 2;
          break;
        case 2:
          page = 1;
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printCalibrateExp(short confCalibrateExp) {
  if (confCalibrateExp < 10) {
    lcd.setCursor(12,1);
    lcd.print(F("    "));
    lcd.setCursor(11,1);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 100) {
    lcd.setCursor(13,1);
    lcd.print(F("   "));
    lcd.setCursor(11,1);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 1000) {
    lcd.setCursor(14,1);
    lcd.print(F("  "));
    lcd.setCursor(11,1);
    lcd.print(confCalibrateExp);
  } else {
    lcd.setCursor(15,1);
    lcd.print(F(" "));
    lcd.setCursor(11,1);
    lcd.print(confCalibrateExp);
  }
}

void confCalibrateExpDown(byte pedalNumber) {
  short confCalibrateExpDown;
  EEPROM.get(10+(4*pedalNumber),confCalibrateExpDown);
  short nCurrentPotValueTemp = analogRead(pedalNumber);
  lcd.setCursor(0,1);
  lcd.print(F("Exp  Down: "));
  lcd.setCursor(3,1);
  lcd.print(pedalNumber+1);
  printCalibrateExp(confCalibrateExpDown);
  checkMenuButtonRelease();
  while (true) {
    short nCurrentPotValue = analogRead(pedalNumber);
    if(abs(nCurrentPotValue - nCurrentPotValueTemp) > POT_THRESHOLD) {
      confCalibrateExpDown = nCurrentPotValue;
      printCalibrateExp(confCalibrateExpDown);
      nCurrentPotValueTemp = nCurrentPotValue;
    }
    if(checkMenuButton(save_button)){
      if (expDown[pedalNumber] != confCalibrateExpDown) {
        EEPROM.put(10+(4*pedalNumber),confCalibrateExpDown);
        expDown[pedalNumber] = confCalibrateExpDown;
      }
      confCalibrateExpUp(pedalNumber);
      return;
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void confCalibrateExpUp(byte pedalNumber) {
  short confCalibrateExpUp;
  EEPROM.get((10+(4*pedalNumber))+2,confCalibrateExpUp);
  short nCurrentPotValueTemp = analogRead(pedalNumber);
  lcd.setCursor(0,1);
  lcd.print(F("Exp  Up: "));
  lcd.setCursor(3,1);
  lcd.print(pedalNumber+1);
  printCalibrateExp(confCalibrateExpUp);
  checkMenuButtonRelease();
  while (true) {
    short nCurrentPotValue = analogRead(pedalNumber);
    if(abs(nCurrentPotValue - nCurrentPotValueTemp) > POT_THRESHOLD) {
      confCalibrateExpUp = nCurrentPotValue;
      printCalibrateExp(confCalibrateExpUp);
      nCurrentPotValueTemp = nCurrentPotValue;
    }
    if(checkMenuButton(save_button)){
      if (expUp[pedalNumber] != confCalibrateExpUp) {
        EEPROM.put((10+(4*pedalNumber))+2,confCalibrateExpUp);
        expUp[pedalNumber] = confCalibrateExpUp;
      }
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printRing(byte confRing) {
  lcd.setCursor(10,1);
  lcd.print(F("      "));
  lcd.setCursor(10,1);
  lcd.print(confRing);
}

void confMenuRingBright() {
  byte confRingBright = EEPROM[4];
  lcd.setCursor(0,1);
  lcd.print(F("Ring brt: "));
  printRing(confRingBright);
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(6, confRingBright);
      ringBright = confRingBright;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confRingBright > 25) {
        confRingBright -= 5;
      } else {
        confRingBright = 100;
      }
      printRing(confRingBright);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confRingBright < 100) {
        confRingBright += 5;
      } else {
        confRingBright = 25;
      }
      printRing(confRingBright);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void confMenuRingDim() {
  byte confRingDim = EEPROM[5];
  lcd.setCursor(0,1);
  lcd.print(F("Ring dim: "));
  printRing(confRingDim);
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(7, confRingDim);
      ringDim = confRingDim;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confRingDim > 0) {
        confRingDim -= 5;
      } else {
        confRingDim = 50;
      }
      printRing(confRingDim);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confRingDim < 50) {
        confRingDim += 5;
      } else {
        confRingDim = 0;
      }
      printRing(confRingDim);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printOmniPort(byte confOmniPort) {
  lcd.setCursor(5,1);
  lcd.print(F("           "));
  lcd.setCursor(5,1);
  switch (confOmniPort) {
    case 0:
      lcd.print(F("None"));
      break;
    case 1:
      lcd.print(F("Expression"));
      break;
    case 2:
      lcd.print(F("FxdSwitch1"));
      break;
    case 3:
      lcd.print(F("FxdSwitch2"));
      break;
  }
}
void confMenuOmniPort(byte portNumber) {
  byte confOmniPort = EEPROM[6+portNumber];
  lcd.setCursor(0,1);
  lcd.print(F("OP : "));
  lcd.setCursor(2,1);
  lcd.print(portNumber+1);
  printOmniPort(confOmniPort);
  checkMenuButtonRelease();

  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(6+portNumber, confOmniPort);
      portType[portNumber] = confOmniPort;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confOmniPort > 0) {
        confOmniPort -= 1;
      } else {
        confOmniPort = 3;
      }
      printOmniPort(confOmniPort);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confOmniPort < 3) {
        confOmniPort += 1;
      } else {
        confOmniPort = 0;
      }
      printOmniPort(confOmniPort);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printNotificationTime(byte confNotificationTime) {
  lcd.setCursor(8,1);
  lcd.print(F("        "));
  lcd.setCursor(9,1);
  lcd.print(confNotificationTime * 10);
}

void confMenuNotificationTime() {
  byte confNotificationTime = EEPROM[3];
  lcd.setCursor(0,1);
  lcd.print(F("N Time: "));
  printNotificationTime(confNotificationTime);
  checkMenuButtonRelease();

  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(3, confNotificationTime);
      notificationTime = confNotificationTime * 10;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confNotificationTime > 0) {
        confNotificationTime -= 5;
      } else {
        confNotificationTime = 100;
      }
      printNotificationTime(confNotificationTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confNotificationTime < 100) {
        confNotificationTime += 5;
      } else {
        confNotificationTime = 0;
      }
      printNotificationTime(confNotificationTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printLongPressTime(byte confLongPressTime) {
  lcd.setCursor(9,1);
  lcd.print(F("       "));
  lcd.setCursor(9,1);
  lcd.print(confLongPressTime * 10);
}

void confMenuLongPressTime() {
  byte confLongPressTime = EEPROM[2];
  lcd.setCursor(0,1);
  lcd.print(F("LP Time: "));
  printLongPressTime(confLongPressTime);
  checkMenuButtonRelease();
  
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(2, confLongPressTime);
      longPressTime = confLongPressTime * 10;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confLongPressTime > 10) {
        confLongPressTime -= 5;
      } else {
        confLongPressTime = 120;
      }
      printLongPressTime(confLongPressTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confLongPressTime < 120) {
        confLongPressTime += 5;
      } else {
        confLongPressTime = 10;
      }
      printLongPressTime(confLongPressTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printDebounceTime(byte confDebounceTime) {
  lcd.setCursor(11,1);
  lcd.print(F("     "));
  lcd.setCursor(11,1);
  lcd.print(confDebounceTime * 10);
}

void confMenuDebounceTime() {
  byte confDebounceTime = EEPROM[1];
  lcd.setCursor(0,1);
  lcd.print(F("Dbnc Time: "));
  printDebounceTime(confDebounceTime);
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(1, confDebounceTime);
      debounceTime = confDebounceTime * 10;
      lcd.setCursor(0,1);
      lcd.print(F("                "));
      lcd.setCursor(0,1);
      lcd.print(F("Saved"));
      delay(800);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confDebounceTime > 5) {
        confDebounceTime -= 1;
      } else {
        confDebounceTime = 30;
      }
      printDebounceTime(confDebounceTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confDebounceTime < 30) {
        confDebounceTime += 1;
      } else {
        confDebounceTime = 5;
      }
      printDebounceTime(confDebounceTime);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}
