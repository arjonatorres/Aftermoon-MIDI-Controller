#include <Adafruit_NeoPixel.h>
#include <MIDI.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C_40x4.h>
#include <extEEPROM.h>
#include "Watchdog_t4.h"

#define n_messages 8
#define n_presets 8
#define n_banks 12
#define n_values 4
#define file_div_size 200
#define save_button 2
#define prev_button 3
#define next_button 4
#define exit_button 5
#define OMNIPORT_NUMBER 2
#define EXP_TYPE 1
#define FXD1_SWITCH_TYPE 2
#define FXD2_SWITCH_TYPE 3
#define MAIN_MSG_TIME 800
#define SCENE_NUMBER 8


// Settings
boolean sendMIDIMonitor = false;
byte portType[OMNIPORT_NUMBER];
byte ringBright;
byte ringDim;
byte requestFm3Scenes;
boolean editMode = false;
boolean setActualScene = false;
unsigned int debounceTime;
unsigned int longPressTime;
unsigned int notificationTime;
unsigned long pressedTime  = 0;
short expDown[OMNIPORT_NUMBER];
short expUp[OMNIPORT_NUMBER];
char presetsName[16] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P'};
WDT_T4<WDT1> wdt;


// EEPROM
const int eepromAddress = 0x50;
extEEPROM myEEPROM(kbits_256, 1, 64);
byte i2cStat;


// MIDI
MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);


// Expression Pedals
const short POT_THRESHOLD = 7;        // Threshold amount to guard against false values

// LCD
LiquidCrystal_I2C_40x4 lcd(0x3F, 40, 2); // 2 Líneas por módulo
//LiquidCrystal_I2C lcd(0x3F, 3, 1, 0, 4, 5, 6, 7);

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
  //{0,191,255},    // 6 - Cyan
  //{0,172,230},    // 6 - Cyan
  {0,153,204},    // 6 - Cyan
  {128,0,128},    // 7 - Purple
  {255,255,255},  // 8 - White
  {255,105,180},  // 9 - Pink
  {0,139,139},    // 10 - Turquoise
  {173,255,47}    // 11 - Lime
};

// FM3 Effects
int fm3Effects[] = {
  46,   // ID_COMP1
  47,   // ID_COMP2
  50,   // ID_GRAPHEQ1
  51,   // ID_GRAPHEQ2
  54,   // ID_PARAEQ1
  55,   // ID_PARAEQ2
  58,   // ID_DISTORT1
  62,   // ID_CAB1
  66,   // ID_REVERB1
  70,   // ID_DELAY1
  71,   // ID_DELAY2
  74,   // ID_MULTITAP1
  78,   // ID_CHORUS1
  79,   // ID_CHORUS2
  82,   // ID_FLANGER
  83,   // ID_FLANGER2
  86,   // ID_ROTARY1
  87,   // ID_ROTARY2
  90,   // ID_PHASER1
  91,   // ID_PHASER2
  94,   // ID_WAH1
  95,   // ID_WAH2
  98,   // ID_FORMANT1
  99,   // ID_FORMANT2
  102,  // ID_VOLUME1
  103,  // ID_VOLUME2
  106,  // ID_TREMOLO1
  107,  // ID_TREMOLO2
  110,  // ID_PITCH1
  114,  // ID_FILTER1
  115,  // ID_FILTER2
  116,  // ID_FILTER3
  117,  // ID_FILTER4
  118,  // ID_FUZZ1
  119,  // ID_FUZZ2
  122,  // ID_ENHANCER1
  123,  // ID_ENHANCER2
  126,  // ID_MIXER1
  127,  // ID_MIXER2
  128,  // ID_MIXER3
  129,  // ID_MIXER4
  130,  // ID_SYNTH1
  138,  // ID_MEGATAP1
  146,  // ID_GATE1
  147,  // ID_GATE2
  150,  // ID_RINGMOD1
  154,  // ID_MULTICOMP1
  158,  // ID_TENTAP1
  162,  // ID_RESONATOR1
  163,  // ID_RESONATOR2
  166,  // ID_LOOPER1
  178,  // ID_PLEX1
  182,  // ID_FBSEND1
  183,  // ID_FBSEND2
  186,  // ID_FBRETURN1
  187,  // ID_FBRETURN2
  190,  // ID_MIDIBLOCK
  191,  // ID_MULTIPLEXER1
  192,  // ID_MULTIPLEXER2
  };

// Data
byte bankNumber;
byte pageNumber;
byte buttonNumber;
byte sceneNumber;
byte presetBankNumber;
byte presetPageNumber;
byte presetButtonNumber;
byte presetSPNumber;      // 0 Si el preset es SP, 1 si es LP

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
    byte presetConf;                  // Bit 6 - lpToggleMode, Bit 5 - spToggleMode, Bit 1 lp Preset Type, Bit 0 sp Preset Type
    char longName[25] = "EMPTY";
    char pShortName[9] = "EMPTY";
    char pToggleName[9] = "EMPTY";
    char lpShortName[9] = "EMPTY";
    char lpToggleName[9] = "EMPTY";
    byte spColor;                       // Bit 6 - Color type (Off, Dimmer), Bit 5-0 color
    byte lpColor;                       // Bit 6 - Color type (Off, Dimmer), Bit 5-0 color
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
    byte posSingleStatus = 1; // 0 - OFF / 1 - ON
    byte posLongStatus = 1;   // 0 - OFF / 1 - ON
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


void setup() {
  for(int i=2; i<=9; i++){
    pinMode(i, INPUT);
  }
  
  MIDI.begin();

  //lcd.begin(16,2);//Defining 16 columns and 2 rows of lcd display
  lcd.init();
  //lcd.backlight();//To Power ON the back light

  i2cStat = myEEPROM.begin(extEEPROM::twiClock400kHz);
  if ( i2cStat != 0 ) {
    printMainMsg(13, F("EEPROM r error"), 0);
    while (true) {}
  }

  // Info Menú
  // A+D check (Info Menu)
  if (digitalRead(2)== HIGH && digitalRead(5)== HIGH) {
    showInfoMenu();
    checkMenuButtonRelease();
    infoMenu();
    checkMenuButtonRelease();
    lcd.clear();
  }

  // Factory Reset
  // E+H check (Factory Menu)
  if (digitalRead(6)== HIGH && digitalRead(9)== HIGH) {
    showFactoryMenu();
    checkMenuButtonRelease();
    factoryMenu();
    checkMenuButtonRelease();
    lcd.clear();
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
  requestFm3Scenes = EEPROM[31];
  
  bankNumber = EEPROM[0];
  pageNumber = 1;
  buttonNumber = 1;
  presetBankNumber = 0;
  presetPageNumber = 0;
  presetButtonNumber = 0;
  presetSPNumber = 0;

  myEEPROM.read(0,(byte*)&data, sizeof(data));
  //myEEPROM.write(0,(byte*)&data, sizeof(data));
  usbMIDI.setHandleSystemExclusive(OnUSBSysEx);

  pixels.begin();
  if (EEPROM[30] == 0) {
    pixels.setBrightness(0);
  } else {
    pixels.setBrightness((EEPROM[30]*32)-1);
  }
  drawColors();
  lcdChangeAll();
}

void loop() {
  usbMIDI.read();
  checkButtons();
  checkOmniPorts();
}

void drawColors() {
  for(int i=0; i<8; i++){
    // SP Colors
    byte r;
    byte g;
    byte b;
    byte spPresetType = bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[i].presetConf, 0);
    int spColorNumber = 0b00111111&(data.bank[bankNumber-1].page[pageNumber-1].preset[i].spColor);
    int lpColorNumber = 0b00111111&(data.bank[bankNumber-1].page[pageNumber-1].preset[i].lpColor);
    byte spColorType = bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[i].spColor, 6);
    r = colors[spColorNumber].r;
    g = colors[spColorNumber].g;
    b = colors[spColorNumber].b;
    
    if (spPresetType == 0){
      if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingle == 0) {
        if (spColorType == 0) {
          for(int j=0; j<3; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
          }
          if (lpColorNumber == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          }
        } else {
          if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingleStatus == 0) {
            for(int j=0; j<3; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
            if (lpColorNumber == 0) {
              for(int j=4; j<7; j++){
                pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
              }
            }
          } else {
            for(int j=0; j<3; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
            }
            if (lpColorNumber == 0) {
              for(int j=4; j<7; j++){
                pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
              }
            }
          }
          
        }
      } else {
        if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingleStatus == 0) {
          for(int j=0; j<3; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
          }
          if (lpColorNumber == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          }
        } else {
          for(int j=0; j<3; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
          }
          if (lpColorNumber == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
            }
          }
        }
        
      }
    } else if (spPresetType == 1) {
      if (bankNumber == presetBankNumber && pageNumber == presetPageNumber && (i+1) == presetButtonNumber && presetSPNumber == 0) {
        for(int j=0; j<3; j++){
          pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
        }
        if (lpColorNumber == 0) {
          for(int j=4; j<7; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
          }
        }
      } else {
        if (spColorType == 0) {
          for(int j=0; j<3; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
          }
          if (lpColorNumber == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          }
        } else {
          for(int j=0; j<3; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
          }
          if (lpColorNumber == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
            }
          }
        }
      }
    }
    if (lpColorNumber != 0) {
      byte lpPresetType = bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[i].presetConf, 1);
      byte lpColorType = bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[i].lpColor, 6);
      r = colors[lpColorNumber-1].r;
      g = colors[lpColorNumber-1].g;
      b = colors[lpColorNumber-1].b;


      if (lpPresetType == 0){
        if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posLong == 0) {
          if (lpColorType == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          } else {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
            }
          }
        } else {
          for(int j=4; j<7; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
          }
        }
      } else if (lpPresetType == 1) {
        if (bankNumber == presetBankNumber && pageNumber == presetPageNumber && (i+1) == presetButtonNumber && presetSPNumber == 1) {
          for(int j=4; j<7; j++){
            pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
          }
        } else {
          if (lpColorType == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          } else {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
            }
          }
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
  lcd.setCursor(39,1);
  lcd.print(F("%"));
  if (percentValue < 10) {
    lcd.setCursor(36,1);
    lcd.print(F("  "));
    lcd.setCursor(38,1);
    lcd.print(percentValue);
  } else if (percentValue >= 10 && percentValue < 100) {
    lcd.setCursor(36,1);
    lcd.print(F(" "));
    lcd.setCursor(37,1);
    lcd.print(percentValue);
  } else {
    lcd.setCursor(36,1);
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
            printMainMsg(14, F("EDIT MODE OFF"), MAIN_MSG_TIME);
            lcdChangeAll();
          } else {
            editMode = true;
            printMainMsg(14, F("EDIT MODE ON"), MAIN_MSG_TIME);
            lcdChangeAll();
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
            lcdChangeAll();
            drawColors();
            checkMenuButtonRelease();
          } else {
            printMainMsg(10, F("EXIT EDIT MODE FIRST"), MAIN_MSG_TIME);
            lcdChangeAll();
          }
        } else {
          btnPressed(i);
          // Trigger Release All Action
          checkMsg(9);
          drawColors();
          //byte presetType = 0b00011111&(data.bank[bankNumber-1].page[pageNumber-1].preset[i-2].presetConf);
          //if (presetType == 0) {
            //lcdPreset();
            lcdPresetNames();
          //} else {
          //  lcdPresetNames();
          //}
        }
      }
    }
  }
}

void btnPressed(byte i) {
  buttonNumber = i-1;
  bool haveLPMsg = checkLPMsg();
  if (editMode) {
    sendUSBPresetData();
  }
  // Trigger Press Action
  checkMsg(1);

  if (haveLPMsg) {
    if (buttonNumber <= 4) {
      lcd.setCursor(((buttonNumber-1)*10),3);
    } else {
      lcd.setCursor(((buttonNumber-1)*10),0);
    }
    if (posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong == 0 || (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 6) == 0) ) {
      printPresetPos(buttonNumber, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].lpShortName);
    } else {
      printPresetPos(buttonNumber, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].lpToggleName);
    }
  }
  
  while (true) {
    // Long Press
    if ((millis() - pressedTime) > longPressTime) {
      // Trigger Long Press Action
      checkMsg(3);
      if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 1) == 1) {
        presetBankNumber = bankNumber;
        presetPageNumber = pageNumber;
        presetButtonNumber = buttonNumber;
        presetSPNumber = 1;
        lcdPreset();
      }
      // Change Toggle
      if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 6) == 1) {
        if (posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong == 0) {
          posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong = 1;
        } else {
          posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong = 0;
        }
      }
      drawColors();
      
      while (true) {
        if (digitalRead(i)== LOW) {
          // Trigger Long Press Release Action
          checkMsg(4);
          return;
        }
      }
    }
    // Release
    if (digitalRead(i)== LOW) {
      if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 0) == 1) {
        presetBankNumber = bankNumber;
        presetPageNumber = pageNumber;
        presetButtonNumber = buttonNumber;
        presetSPNumber = 0;
        lcdPreset();
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

bool checkLPMsg() {
  for (byte i=0; i < n_messages; i++) {
    if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].action == 3 || data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].action == 4) {
      return true;
    }
  }
  return false;
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
      lcdChangeAll();
      break;
    // Bank Down
    case 11:
      pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]) + 1;
      bankDown();
      lcdChangeAll();
      break;
    // Bank Jump
    case 13:
      bankNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]) + 1;
      lcdChangeAll();
      break;
    // Toggle Page
    case 14:
      togglePag();
      break;
    // Set Toggle Single
    case 15:
    {
      byte actionToggle = 0b00000011&(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      byte actionBankNumber = (0b01111100&(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0])) >> 2;
      if (actionBankNumber == 0) {
        actionBankNumber = bankNumber;
      }
      //byte actionToggle = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      byte num1 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      byte num2 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      byte num3 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[3]);
      
      for(int i=0; i<7; i++){
        if (bitValue(num1, i) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 1;
          } else if (actionToggle == 2) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 0;
          }
        } else if ((bitValue(num1, i) == 0) && (actionToggle == 0)) {
          posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 0;
        }
      }
      for(int i=7; i<14; i++){
        if (bitValue(num2, (i-7)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            if (i < n_presets) {
              posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 1;
            } else {
              posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posSingle = 1;
            }
          } else if (actionToggle == 2) {
            if (i < n_presets) {
              posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 0;
            } else {
              posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posSingle = 0;
            }
          }
        } else if ((bitValue(num2, (i-7)) == 0) && (actionToggle == 0)) {
          if (i < n_presets) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posSingle = 0;
          } else {
            posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posSingle = 0;
          }
        }
      }
      for(int i=14; i<16; i++){
        if (bitValue(num3, (i-14)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posSingle = 1;
          } else if (actionToggle == 2) {
            posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posSingle = 0;
          }
        } else if ((bitValue(num3, (i-14)) == 0) && (actionToggle == 0)) {
          posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posSingle = 0;
        }
      }
      break;
    }
    // Set Toggle Long
    case 16:
    {
      byte actionToggle = 0b00000011&(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      //byte actionToggle = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
      byte actionBankNumber = (0b01111100&(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0])) >> 2;
      if (actionBankNumber == 0) {
        actionBankNumber = bankNumber;
      }
      byte num1 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      byte num2 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      byte num3 = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[3]);
      
      for(int i=0; i<7; i++){
        if (bitValue(num1, i) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 1;
          } else if (actionToggle == 2) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 0;
          }
        } else if ((bitValue(num1, i) == 0) && (actionToggle == 0)) {
          posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 0;
        }
      }
      for(int i=7; i<14; i++){
        if (bitValue(num2, (i-7)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            if (i < n_presets) {
              posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 1;
            } else {
              posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posLong = 1;
            }
          } else if (actionToggle == 2) {
            if (i < n_presets) {
              posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 0;
            } else {
              posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posLong = 0;
            }
          }
        } else if ((bitValue(num2, (i-7)) == 0) && (actionToggle == 0)) {
          if (i < n_presets) {
            posData.bank[actionBankNumber-1].page[0].preset[i].posLong = 0;
          } else {
            posData.bank[actionBankNumber-1].page[1].preset[(i-n_presets)].posLong = 0;
          }
        }
      }
      for(int i=14; i<16; i++){
        if (bitValue(num3, (i-14)) == 1) {
          if ((actionToggle == 0) || (actionToggle == 1)) {
            posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posLong = 1;
          } else if (actionToggle == 2) {
            posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posLong = 0;
          }
        } else if ((bitValue(num3, (i-14)) == 0) && (actionToggle == 0)) {
          posData.bank[actionBankNumber-1].page[1].preset[i-n_presets].posLong = 0;
        }
      }
      break;
    }
    // Delay
    case 23:
      delay((data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0])*10);
      break;
    // FM3 Preset Change
    case 25:
      MIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      if (sendMIDIMonitor && editMode) {
        usbMIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      }
      fm3PresetChange();
      break;
    // FM3 Effect State Change
    case 26:
    {
      byte effectIDIndex = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0];
      int effectID = fm3Effects[effectIDIndex];
      byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0A, 0x00, 0x00, 0x00 };
      dataMiddle[6] = effectID&0x7F;
      dataMiddle[7] = effectID >> 7;
      dataMiddle[8] = posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle;
      byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
      byte dataCRC[] = { checksum };
      byte dataEnd[] = { 0xF7 };

      MIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
      MIDI.sendSysEx(1, dataCRC, true);
      MIDI.sendSysEx(1, dataEnd, true);
      if (editMode) {
        usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
        usbMIDI.sendSysEx(1, dataCRC, true);
        usbMIDI.sendSysEx(1, dataEnd, true);
      }
      break;
    }
    // FM3 Scene Change
    case 27:
    {
      byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0C, 0x00 };
      dataMiddle[6] = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0];
      byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
      byte dataCRC[] = { checksum };
      byte dataEnd[] = { 0xF7 };

      MIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
      MIDI.sendSysEx(1, dataCRC, true);
      MIDI.sendSysEx(1, dataEnd, true);
      if (editMode) {
        usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
        usbMIDI.sendSysEx(1, dataCRC, true);
        usbMIDI.sendSysEx(1, dataEnd, true);
      }
      
      // Recorremos los presets para setear las escenas donde corresponda
      for(byte i_bank=0; i_bank<n_banks; i_bank++){
        for(byte i_page=0; i_page<2; i_page++){
          for(byte i_preset=0; i_preset<n_presets; i_preset++){
            for(byte i_message=0; i_message<n_messages; i_message++){
              if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 27) {
                posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
              }
            }
          }
        }
      }
      break;
    }
    // FM3 Channel Change
    case 28:
    {
      byte effectIDIndex = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0];
      int effectID = fm3Effects[effectIDIndex];
      byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0B, 0x00, 0x00, 0x00 };
      dataMiddle[6] = effectID&0x7F;
      dataMiddle[7] = effectID >> 7;
      dataMiddle[8] = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1];
      byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
      byte dataCRC[] = { checksum };
      byte dataEnd[] = { 0xF7 };

      MIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
      MIDI.sendSysEx(1, dataCRC, true);
      MIDI.sendSysEx(1, dataEnd, true);
      if (editMode) {
        usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
        usbMIDI.sendSysEx(1, dataCRC, true);
        usbMIDI.sendSysEx(1, dataEnd, true);
      }
      // Recorremos los presets para setear las escenas donde corresponda
      for(byte i_bank=0; i_bank<n_banks; i_bank++){
        for(byte i_page=0; i_page<2; i_page++){
          for(byte i_preset=0; i_preset<n_presets; i_preset++){
            for(byte i_message=0; i_message<n_messages; i_message++){
              if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 28) {
                posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
              }
            }
          }
        }
      }
      break;
    }
  }
}

void fm3PresetChange() {
  // Solicitamos el estado de todos los efectos al FM3
  byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x13 };
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };

  MIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  MIDI.sendSysEx(1, dataCRC, true);
  MIDI.sendSysEx(1, dataEnd, true);
  if (editMode) {
    usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
    usbMIDI.sendSysEx(1, dataCRC, true);
    usbMIDI.sendSysEx(1, dataEnd, true);
  }
}

void sendUSBPresetData() {
  int arraySize = sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]) + 5;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 0x01;
  dataMiddle[2] = bankNumber;
  dataMiddle[3] = pageNumber;
  dataMiddle[4] = buttonNumber;
  memcpy(&dataMiddle[5], (byte*)&data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1], sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };

  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendCurrentBankData() {
  int arraySize = sizeof(data.bank[bankNumber-1]) + 3;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 10;
  dataMiddle[2] = bankNumber;
  memcpy(&dataMiddle[3], (byte*)&data.bank[bankNumber-1], sizeof(data.bank[bankNumber-1]));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };

  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendAllBanksData(byte bankNumberRx) {
  int arraySize = sizeof(data.bank[bankNumberRx-1]) + 3;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 12;
  dataMiddle[2] = bankNumberRx;
  memcpy(&dataMiddle[3], (byte*)&data.bank[bankNumberRx-1], sizeof(data.bank[bankNumberRx-1]));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };

  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendUSBBankData() {
  int arraySize = sizeof(data.bank[bankNumber-1].bankName) + 2;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 0x02;
  memcpy(&dataMiddle[2], (byte*)&data.bank[bankNumber-1].bankName, sizeof(data.bank[bankNumber-1].bankName));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };
  
  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendSettingsData() {
  byte eepromData[] = { EEPROM[1], EEPROM[2], EEPROM[3], EEPROM[4], EEPROM[5], EEPROM[30], EEPROM[6], EEPROM[7], EEPROM[31] };
  int arraySize = sizeof(eepromData) + 2;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 0x03;
  memcpy(&dataMiddle[2], (byte*)&eepromData, sizeof(eepromData));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };
  
  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendUSBExpData(byte pedalNumber) {
  int arraySize = sizeof(data.bank[bankNumber-1].port[pedalNumber]) + 4;
  byte dataMiddle[arraySize] = {};
  dataMiddle[0] = 0xF0;
  dataMiddle[1] = 0x09;
  dataMiddle[2] = bankNumber;
  dataMiddle[3] = pedalNumber;
  memcpy(&dataMiddle[4], (byte*)&data.bank[bankNumber-1].port[pedalNumber], sizeof(data.bank[bankNumber-1].port[pedalNumber]));
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };
  
  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void OnSysEx(byte* readData, unsigned sizeofsysex) {
  if (readData[1] == 0x00 && readData[2] == 0x01 && readData[3] == 0x74 && readData[4] == 0x11) {
    switch(readData[5]) {
      // Receive all effects status
      case 0x13:
      {
        int z = 0;
        for(unsigned int i=6; i<sizeofsysex-2; i+=3) {
          z++;
        }

        byte effectIDs[z*2];
        int y = 0;
        for(unsigned int i=6; i<sizeofsysex-2; i+=3){
          int effectID = readData[i];
          if (readData[i+1] != 0) {
            int effectIDSec = (int)readData[i+1];
            effectID = (effectIDSec << 7) + effectID;
          }
          byte sizeFm3Effects = sizeof(fm3Effects) / sizeof(int);
          for(byte j=0; j<sizeFm3Effects; j++){
            if (effectID == fm3Effects[j]) {
              effectIDs[y] = j;
              effectIDs[y+1] = readData[i+2];
              y+=2;
              break;
            }
          }
        }

        // Recorremos los presets para setear los efectos donde corresponda y los canales
        for(byte i_bank=0; i_bank<n_banks; i_bank++){
          for(byte i_page=0; i_page<2; i_page++){
            for(byte i_preset=0; i_preset<n_presets; i_preset++){
              for(byte i_message=0; i_message<n_messages; i_message++){
                // Seteamos el estado de los efectos y los canales
                byte effectType = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type;
                if (effectType == 26 || effectType == 28) {
                  bool effectFind = false;
                  for(byte k=0; k<(z*2); k+=2) {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == effectIDs[k]) {
                      effectFind = true;
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingleStatus = 1;
                      if ((bitValue(effectIDs[k+1], 0) == 0 && effectType == 26) || ((((effectIDs[k+1] >> 1)&0x03) == data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[1]) && effectType == 28)) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                      }
                    }
                  }
                  if (!effectFind && effectType == 26) {
                    posData.bank[i_bank].page[i_page].preset[i_preset].posSingleStatus = 0;
                  }
                }
              }
            }
          }
        }
        if (requestFm3Scenes) {
          setActualScene = true;
          fm3Scenes(0x7F);
        }
        return;
      }
      // Receive Scene Names
      case 0x0E:
      {
        if (setActualScene) {
          sceneNumber = readData[6];
          setActualScene = false;
          // Recorremos los presets para setear las escenas donde corresponda
          for(byte i_bank=0; i_bank<n_banks; i_bank++){
            for(byte i_page=0; i_page<2; i_page++){
              for(byte i_preset=0; i_preset<n_presets; i_preset++){
                for(byte i_message=0; i_message<n_messages; i_message++){
                  if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 27) {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == readData[6]) {
                      if (readData[7] == 32) {
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].longName, "Scene ", 24);
                        data.bank[i_bank].page[i_page].preset[i_preset].longName[6] = (readData[6]+1) + '0';
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, "Scene ", 8);
                        data.bank[i_bank].page[i_page].preset[i_preset].pShortName[6] = (readData[6]+1) + '0';
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, "Scene ", 8);
                        data.bank[i_bank].page[i_page].preset[i_preset].pToggleName[6] = (readData[6]+1) + '0';
                      } else {
                        char rd2[24];
                        for (int i=0; i<24; i++) {
                            rd2[i] = readData[7+i];
                        }
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].longName, rd2, 24);
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, rd2, 8);
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, rd2, 8);
                      }
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                    } else {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                    }
                  }
                }
              }
            }
          }
          if (sceneNumber == 0) {
            fm3Scenes(1);
          } else {
            fm3Scenes(0);
          }
          return;
        }
        // Recorremos los presets para setear las escenas donde corresponda
        for(byte i_bank=0; i_bank<n_banks; i_bank++){
          for(byte i_page=0; i_page<2; i_page++){
            for(byte i_preset=0; i_preset<n_presets; i_preset++){
              for(byte i_message=0; i_message<n_messages; i_message++){
                if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 27) {
                  if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == readData[6]) {
                    if (readData[7] == 32) {
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].longName, "Scene ", 24);
                      data.bank[i_bank].page[i_page].preset[i_preset].longName[6] = (readData[6]+1) + '0';
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, "Scene ", 8);
                      data.bank[i_bank].page[i_page].preset[i_preset].pShortName[6] = (readData[6]+1) + '0';
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, "Scene ", 8);
                      data.bank[i_bank].page[i_page].preset[i_preset].pToggleName[6] = (readData[6]+1) + '0';
                    } else {
                      char rd2[24];
                      for (int i=0; i<24; i++) {
                          rd2[i] = readData[7+i];
                      }
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].longName, rd2, 24);
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, rd2, 8);
                      strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, rd2, 8);
                    }
                  }
                }
              }
            }
          }
        }
  
        if (sceneNumber != (readData[6] + 1)) {
          if ((readData[6]+1) < 8) {
            fm3Scenes(readData[6] + 1);
            return;
          }
        } else {
          if ((readData[6]+2) < 8) {
            fm3Scenes(readData[6] + 2);
            return;
          }
        }
        break;
      }
    }
  }
  drawColors();
  lcdChangeAll();
}

void fm3Scenes(byte scene) {
  // Solicitamos el estado de la escena actual del FM3
  byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0E, scene };
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };

  MIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  MIDI.sendSysEx(1, dataCRC, true);
  MIDI.sendSysEx(1, dataEnd, true);
  if (editMode) {
    usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
    usbMIDI.sendSysEx(1, dataCRC, true);
    usbMIDI.sendSysEx(1, dataEnd, true);
  }
}


void OnUSBSysEx(byte* readData, unsigned sizeofsysex) {
  byte checksumOrigin = XORChecksum8((byte*)&readData[0], sizeofsysex-2);
  byte checksum = readData[sizeofsysex-2];
  if (checksumOrigin != checksum) {
    lcd.clear();
    //lcd.setCursor(12,1);
    //lcd.print(F("Error in Data..."));
    lcd.setCursor(0,1);
    lcd.print(F("ChecksumOrigin: "));
    lcd.setCursor(16,1);
    lcd.print(checksumOrigin);
    lcd.setCursor(0,2);
    lcd.print(F("Checksum: "));
    lcd.setCursor(10,2);
    lcd.print(checksum);
    lcd.setCursor(0,3);
    lcd.print(F("Sizeofsysex: "));
    lcd.setCursor(13,3);
    lcd.print(sizeofsysex);
    delay(800);
    drawColors();
    lcdChangeAll();
    return;
  }
  unsigned sizeOfData = 0;
  static byte restoreBankCont = 0;
  static byte restorePaqSize = 0;
  //static struct Bank bankTemp;
  static byte bankTemp[sizeof(data.bank[bankNumber-1])];
  static byte allBanksTemp[sizeof(data)];
  static int allBanksCont = 0;
  if (editMode) {
    sendMIDIMonitor = false;
    int bankOffset = (bankNumber-1)*(sizeof(data.bank[bankNumber-1]));
    if (readData[1] != 11 && (readData[1] != 12 || (readData[1] == 12 && readData[2] == 1)) && readData[1] != 13 && readData[1] != 14) {
      printMainMsg(12, F("Communicating..."), 0);
    }
  
    switch (readData[1]) {
      // TODO - Borrar cuando se habilite el MIDI normal
      case 0:
        OnSysEx(readData, sizeofsysex);
        return;
      // Save Preset Data
      case 1:
        sizeOfData = sizeof(struct Preset) + 7;
        if (sizeofsysex == sizeOfData) {
          int presetOffset;
          bankNumber = readData[2];
          pageNumber = readData[3];
          buttonNumber = readData[4];
          if (pageNumber == 1) {
            presetOffset = (sizeof(data.bank[bankNumber-1].bankName)) + ((buttonNumber-1)*(sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1])));
          } else {
            presetOffset = (sizeof(data.bank[bankNumber-1].bankName)) + (sizeof(data.bank[bankNumber-1].page[0])) + ((buttonNumber-1)*(sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1])));
          }
          
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1], &readData[5], sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]));
      
          // Guardado en EEPROM
          i2cStat = myEEPROM.write((bankOffset + presetOffset),(byte*)&readData[5], sizeof(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1]));
          if ( i2cStat != 0 ) {
            printMainMsg(13, F("EEPROM w error"), 0);
            while (true) {}
          }
          printMainMsg(14, F("Saved Preset"), MAIN_MSG_TIME);
        }
        break;
        
      // Save Bank Data
      case 2:
        sizeOfData = sizeof(data.bank[bankNumber-1].bankName) + 5;
        if (sizeofsysex == sizeOfData) {
          bankNumber = readData[2];
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].bankName, &readData[3], sizeof(data.bank[bankNumber-1].bankName));
      
          // Guardado en EEPROM
          i2cStat = myEEPROM.write(bankOffset,(byte*)&readData[3], sizeof(data.bank[bankNumber-1].bankName));
          if ( i2cStat != 0 ) {
            printMainMsg(13, F("EEPROM w error"), 0);
            while (true) {}
          }
          printMainMsg(15, F("Saved Bank"), MAIN_MSG_TIME);
        }
        break;
        
      // Preset Data Request
      case 3:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          sendUSBPresetData();
        }
        break;
      // Bank Data Request
      case 4:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          sendUSBBankData();
        }
        break;
      // Settings Data Request
      case 5:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          sendSettingsData();
        }
        break;
      // Save Settings Data
      case 6:
        sizeOfData = 12;
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
          EEPROM.update(30, readData[7]);
          if (EEPROM[30] == 0) {
            pixels.setBrightness(0);
          } else {
            pixels.setBrightness((EEPROM[30]*32)-1);
          }
          EEPROM.update(6, readData[8]);
          portType[0] =  readData[8];
          EEPROM.update(7, readData[9]);
          portType[1] =  readData[9];
          EEPROM.update(31, readData[10]);
          requestFm3Scenes =  readData[10];

          printMainMsg(13, F("Saved Settings"), MAIN_MSG_TIME);
        }
        break;
      // MIDI Monitor
      case 7:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          sendMIDIMonitor = true;
        }
        break;
      // Save Exp Data
      case 9:
        sizeOfData = sizeof(struct OmniPort) + 6;
        if (sizeofsysex == sizeOfData) {
          bankNumber = readData[2];
          byte expNumber = readData[3];
          int expOffset = (sizeof(data.bank[bankNumber-1].bankName)) + (2*(sizeof(data.bank[bankNumber-1].page[0]))) + ((expNumber)*(sizeof(data.bank[bankNumber-1].port[expNumber])));
          // Guardado en memoria
          memcpy(&data.bank[bankNumber-1].port[expNumber], &readData[4], sizeof(data.bank[bankNumber-1].port[expNumber]));
          // Guardado en EEPROM
          i2cStat = myEEPROM.write((bankOffset + expOffset),(byte*)&readData[4], sizeof(data.bank[bankNumber-1].port[expNumber]));
          if ( i2cStat != 0 ) {
            printMainMsg(13, F("EEPROM w error"), 0);
            while (true) {}
          }
          printMainMsg(12, F("Saved Expression"), MAIN_MSG_TIME);
        }
        break;
      // Backup Send Current Bank
      case 10:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          sendCurrentBankData();
        }
        break;
      // Backup Restore Current Bank
      case 11:
        {
          byte restoreBankContRx = readData[2];
  
          if (restoreBankContRx == 0) {
            restoreBankCont = restoreBankContRx;
            restorePaqSize = readData[3];
            lcd.clear();
            lcd.setCursor(10,1);
            lcd.print(F("Save data in Bank"));
            lcd.setCursor(28,1);
            lcd.print(bankNumber);
            byte qNumber = 29;
            if (bankNumber >= 10) {
              qNumber = 30;
            }
            lcd.setCursor(qNumber,1);
            lcd.print(F("?"));
            lcd.setCursor(0,3);
            lcd.print(F("(  Save  )                    (  Exit  )"));
            while (true) {
              if(checkMenuButton(save_button)){
                // Guardado en memoria
                memcpy(&bankTemp, &readData[4], file_div_size);
  
                printMainMsg(9, F("Receiving Bank Data..."), 500);
                restoreBankCont += 1;
                
                lcd.setCursor(18,2);
                lcd.print(restoreBankCont);
                lcd.setCursor(19,2);
                lcd.print(F("/"));
                lcd.setCursor(20,2);
                lcd.print(restorePaqSize);
                
                sendRestoreBank(restoreBankCont);
                return;
              } else if(checkMenuButton(exit_button)){
                break;
              }
            }
          } else if (restoreBankContRx == restoreBankCont) {
              if (restoreBankContRx != (restorePaqSize-1)) {
                memcpy(&bankTemp[restoreBankContRx * file_div_size], &readData[3], file_div_size);
                restoreBankCont += 1;
                if (restoreBankCont < 10) {
                  lcd.setCursor(18,2);
                } else {
                  lcd.setCursor(17,2);
                }
                lcd.print(restoreBankCont);
                lcd.setCursor(19,2);
                lcd.print(F("/"));
                lcd.setCursor(20,2);
                lcd.print(restorePaqSize);
                delay(100);
                sendRestoreBank(restoreBankCont);
                return;
              } else {
                // Ultimo paquete
                if (((restoreBankContRx * file_div_size) + (sizeofsysex-5)) > sizeof(data.bank[0])) {
                  lcd.clear();
                  lcd.setCursor(8,1);
                  lcd.print(F("Error receiving data..."));
                  delay(1000);
                  break;
                }
                memcpy(&bankTemp[restoreBankContRx * file_div_size], &readData[3], sizeofsysex-5);
                if ((restoreBankCont+1) < 10) {
                  lcd.setCursor(18,2);
                } else {
                  lcd.setCursor(17,2);
                }
                lcd.print(restoreBankCont+1);
                lcd.setCursor(19,2);
                lcd.print(F("/"));
                lcd.setCursor(20,2);
                lcd.print(restorePaqSize);
                
                // Guardado en memoria
                memcpy(&data.bank[bankNumber-1], &bankTemp, sizeof(data.bank[bankNumber-1]));
            
                // Guardado en EEPROM
                i2cStat = myEEPROM.write(bankOffset,(byte*)&data.bank[bankNumber-1], sizeof(data.bank[bankNumber-1]));
                if ( i2cStat != 0 ) {
                  printMainMsg(13, F("EEPROM w error"), 0);
                  while (true) {}
                }
                lcd.clear();
                lcd.setCursor(5,1);
                lcd.print(F("Saved Backup Data in Bank"));
                lcd.setCursor(31,1);
                lcd.print(bankNumber);
                delay(300);
              }
          } else {
            printMainMsg(7, F("Error al recibir los datos"), MAIN_MSG_TIME);
            break;
          }
        }
        break;
      // Backup Send All Banks
      case 12:
        {
          sizeOfData = 5;
          if (sizeofsysex == sizeOfData) {
            byte sendBankContRx = readData[2];
            lcd.clear();
            lcd.setCursor(11,1);
            lcd.print(F("Sending all Banks"));
            if ((sendBankContRx) < 10) {
              lcd.setCursor(18,2);
            } else {
              lcd.setCursor(17,2);
            }
            lcd.print(sendBankContRx);
            lcd.setCursor(19,2);
            lcd.print(F("/"));
            lcd.setCursor(20,2);
            lcd.print(n_banks);

            delay(150);
            sendAllBanksData(sendBankContRx);
            if (sendBankContRx < n_banks) {
              return;
            } else {
              delay(300);
              break;
            }
          }
        }
        break;
      // Restore All Banks Data First Time
      case 13:
        lcd.clear();
        lcd.setCursor(5,1);
        lcd.print(F("Save all banks data? (DANGER)"));
        lcd.setCursor(0,3);
        lcd.print(F("(  Save  )                    (  Exit  )"));
        while (true) {
          if(checkMenuButton(save_button)){
            // Guardado en memoria
            memcpy(&allBanksTemp, &readData[2], file_div_size);

            printMainMsg(9, F("Receiving Bank Data..."), 500);
            allBanksCont = sizeofsysex-4;

            lcd.setCursor(17,2);
            lcd.print(F("0 %"));
            delay(100);
            sendRestoreAllBanks(13);
            return;
          } else if(checkMenuButton(exit_button)){
            break;
          }
        }
        break;
      // Restore All Banks Data X Times
      case 14:
        // Guardado en memoria
        memcpy(&allBanksTemp[allBanksCont], &readData[2], (sizeofsysex-4));

        if ((allBanksCont + (sizeofsysex-4)) > sizeof(data)) {
          lcd.clear();
          lcd.setCursor(8,1);
          lcd.print(F("Error receiving data..."));
          delay(1000);
          break;
        }

        // Último paquete
        if ((allBanksCont + (sizeofsysex-4)) == sizeof(data)) {
          lcd.clear();
          lcd.setCursor(14,1);
          lcd.print(F("Saving..."));
          // Guardado en memoria
          memcpy(&data, &allBanksTemp, sizeof(data));
      
          // Guardado en EEPROM
          i2cStat = myEEPROM.write(0,(byte*)&data, sizeof(data));
          if ( i2cStat != 0 ) {
            printMainMsg(13, F("EEPROM w error"), 0);
            while (true) {}
          }
          lcd.clear();
          lcd.setCursor(4,1);
          lcd.print(F("Saved Backup Data in All Banks"));
          delay(1000);
          break;
        }
        
        allBanksCont += (sizeofsysex-4);

        if (((allBanksCont*100)/sizeof(data)) < 10) {
          lcd.setCursor(17,2);
        } else {
          lcd.setCursor(16,2);
        }
        lcd.print((allBanksCont*100)/sizeof(data));
        lcd.setCursor(18,2);
        lcd.print(F(" %"));
        delay(100);
        sendRestoreAllBanks(14);
        return;
    }
    delay(500);
    drawColors();
    lcdChangeAll();
  } else {
    if (readData[1] == 8) {
      sizeOfData = 5;
      if (sizeofsysex == sizeOfData) {
        editMode = true;
        sendUSBPresetData();
        printMainMsg(14, F("EDIT MODE ON"), MAIN_MSG_TIME);
        lcdChangeAll();
      }
    }
  }
}

void sendRestoreBank (byte restoreBankContTemp) {
  byte dataMiddle[] = { 0xF0, 11, restoreBankContTemp };
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };
  
  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void sendRestoreAllBanks (byte byteData) {
  byte dataMiddle[] = { 0xF0, byteData };
  byte checksum = XORChecksum8((byte*)&dataMiddle, sizeof(dataMiddle));
  byte dataCRC[] = { checksum };
  byte dataEnd[] = { 0xF7 };
  
  usbMIDI.sendSysEx(sizeof(dataMiddle), (byte*)&dataMiddle, true);
  usbMIDI.sendSysEx(1, dataCRC, true);
  usbMIDI.sendSysEx(1, dataEnd, true);
}

void lcdChangeAll() {
  lcd.clear();
  lcdPresetNames();
  lcdBank();
  lcdPreset();
}

void lcdBank() {
  byte bankLCDOffset = 0;
  lcd.setCursor(0,1);
  lcd.print(F("B                               "));
  lcd.setCursor(1,1);
  lcd.print(bankNumber, DEC);
  if (bankNumber >= 10) {
    bankLCDOffset = 1;
  }
  lcd.setCursor(3+bankLCDOffset,1);
  lcd.print(F("P"));
  lcd.setCursor(4+bankLCDOffset,1);
  lcd.print(pageNumber, DEC);
  lcd.setCursor(5+bankLCDOffset,1);
  lcd.print(F(": "));
  lcd.setCursor(7+bankLCDOffset,1);
  lcd.print(F(data.bank[bankNumber-1].bankName));
}

void lcdPreset() {
  lcd.setCursor(0,2);
  lcd.print(F("PST                              "));
  lcd.setCursor(4,2);
  if (presetButtonNumber != 0) {
    if (pageNumber == 1) {
      lcd.print(presetsName[presetButtonNumber-1]);
    } else {
      lcd.print(presetsName[(presetButtonNumber-1)+n_presets]);
    }
  }
  
  lcd.setCursor(5,2);
  lcd.print(F(": "));
  lcd.setCursor(7,2);
  if (presetButtonNumber != 0) {
    lcd.print(data.bank[presetBankNumber-1].page[presetPageNumber-1].preset[presetButtonNumber-1].longName);
  }
}

void lcdPresetNames() {
  for(int i=0; i<4; i++){
    lcd.setCursor((i*10),3);
    if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingle == 0) {
      printPresetPos(i+1, data.bank[bankNumber-1].page[pageNumber-1].preset[i].pShortName);
    } else {
      printPresetPos(i+1, data.bank[bankNumber-1].page[pageNumber-1].preset[i].pToggleName);
    }
  }
  for(int i=4; i<8; i++){
    lcd.setCursor(((i-4)*10),0);
    if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posSingle == 0) {
      printPresetPos(i-3, data.bank[bankNumber-1].page[pageNumber-1].preset[i].pShortName);
    } else {
      printPresetPos(i-3, data.bank[bankNumber-1].page[pageNumber-1].preset[i].pToggleName);
    }
  }
}

void printPresetPos(int pos, char stringVal[]) {
  char stringFinal[9] = "";
  char spaces[9] = "";
  switch (pos) {
    case 1:
    case 2:
      strcat(stringFinal, stringVal);
      //stringFinal = stringVal;
      for (int j=strlen(stringFinal); j<10; j++) {
        strcat(spaces, " "); 
      }
      strcat(stringFinal, spaces);
      break;
    //case 2:
      //strcat(stringFinal, "2");
      //strcat(stringFinal, stringVal);
      //for (int j=strlen(stringFinal); j<10; j++) {
      //  strcat(spaces, "2"); 
      //}
      //strcat(stringFinal, spaces);
      //break;
    //case 3:
      //for (int j=strlen(stringVal); j<9; j++) {
      //  strcat(spaces, "3"); 
      //}
      //strcat(stringFinal, spaces);
      //strcat(stringFinal, stringVal);
      //strcat(stringFinal, "3");
      //break;
    case 3:
    case 4:
      for (int j=strlen(stringVal); j<10; j++) {
        strcat(spaces, " "); 
      }
      strcat(stringFinal, spaces);
      strcat(stringFinal, stringVal);
      break;
  }
  lcd.print(stringFinal);
}

byte bitValue(byte num, byte pos) {
  return ((num >> pos)&0x01);
}

byte setBit(byte num, byte pos) {
  return num |= 1 << pos;
}

void togglePag() {
  if (pageNumber == 1) {
    pageNumber = 2;
  } else {
    pageNumber = 1;
  }
  lcdPresetNames();
  lcdBank();
}

void bankDown() {
  if (bankNumber == 1) {
    bankNumber = n_banks;
  } else {
    bankNumber--;
  }
  EEPROM[0] = bankNumber;
  lcdPresetNames();
  lcdBank();
}

void bankUp() {
  if (bankNumber == n_banks) {
    bankNumber = 1;
  } else {
    bankNumber++;
  }
  EEPROM[0] = bankNumber;
  lcdPresetNames();
  lcdBank();
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

void showInfoMenu() {
  lcd.clear();
  // Fila 1
  lcd.setCursor(0,1);
  lcd.print(F("Bank size: "));
  lcd.setCursor(11,1);
  lcd.print(sizeof(data.bank[0]));
  lcd.setCursor(0,2);
  lcd.print(F("All size: "));
  lcd.setCursor(10,2);
  lcd.print(sizeof(data));
  // Fila 3 (Inferior)
  lcd.setCursor(0,3);
  lcd.print(F("                              (  Exit  )"));
}

void infoMenu() {
  while (true) {
    if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void showFactoryMenu() {
  lcd.clear();
  // Fila 0 (Superior)
  lcd.setCursor(0,0);
  lcd.print(F("( Banks  )(Settings)(  All   )          "));
  // Fila 1
  lcd.setCursor(0,1);
  lcd.print(F("[Factory Reset Menu]"));
  // Fila 3 (Inferior)
  lcd.setCursor(0,3);
  lcd.print(F("                              (  Exit  )"));
}

void factoryMenu() {
  while (true) {
    // Button E (Reset Banks)
    if(checkMenuButton(6)){
      if (resetMenuConfirm(13, F("Reset Banks?"))) {
        printMainMsg(13, F("Resetting..."), 0);
        resetBanks();
        printMainMsg(11, F("Reset Banks Done"), 1200);
        return;
      }
    // Button F (Reset Settings)
    } else if(checkMenuButton(7)){
      if (resetMenuConfirm(12, F("Reset Settings?"))) {
        printMainMsg(13, F("Resetting..."), MAIN_MSG_TIME);
        resetSettings();
        printMainMsg(9, F("Reset Settings Done"), 1200);
        return;
      }
    // Button G (Reset All)
    } else if(checkMenuButton(8)){
      if (resetMenuConfirm(14, F("Reset All?"))) {
        printMainMsg(13, F("Resetting..."), 0);
        resetBanks();
        resetSettings();
        printMainMsg(12, F("Reset All Done"), 1200);
        return;
      }
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void resetBanks() {
  myEEPROM.write(0,(byte*)&data, sizeof(data));
}

void resetSettings() {
  EEPROM[0] = 1; // bankNumber
  EEPROM[1] = 10; // debounceTime
  EEPROM[2] = 50; // longPressTime
  EEPROM[3] = 40; // notificationTime
  EEPROM[4] = 100; // ringBright
  EEPROM[5] = 10; // ringDim
  EEPROM[30] = 2; // allBright
  EEPROM[31] = 0; // requestFm3Scenes

  short confCalibrateExpDown = 0;
  short confCalibrateExpUp = 1023;
  
  for(int i=0; i<OMNIPORT_NUMBER; i++){
    EEPROM[6+i] = 0;
  }
  for(int i=0; i<OMNIPORT_NUMBER; i++){
    EEPROM.put(10+(4*i), confCalibrateExpDown);
    EEPROM.put((10+(4*i))+2, confCalibrateExpUp);
  }
}

boolean resetMenuConfirm(int pos, String msg) {
  lcd.clear();
  lcd.setCursor(pos,1);
  lcd.print(msg);
  lcd.setCursor(1,2);
  lcd.print(F("Press Reset button to confirm (Danger)"));
  lcd.setCursor(0,3);
  lcd.print(F("( Reset  )                    (  Exit  )"));
  while (true) {
    if(checkMenuButton(save_button)){
      checkMenuButtonRelease();
      return true;
    } else if(checkMenuButton(exit_button)){
      showFactoryMenu();
      checkMenuButtonRelease();
      return false;
    }
  }
}

void showConfMenu(byte page) {
  lcd.clear();
  // Fila 0 (Superior)
  lcd.setCursor(0,0);
  switch (page) {
    case 1:
      lcd.print(F("(OmniPor1)(OmniPor2)(ReqScene)(  Page2 )"));
      break;
    case 2:
      lcd.print(F("(Calibr-1)(Calibr-2)          (  Page3 )"));
      break;
    case 3:
      lcd.print(F("                              (  Page1 )"));
      break;
  }
  
  lcd.setCursor(0,1);
  lcd.print(F("[Global Configuration Menu]             "));
  // Fila 3 (Inferior)
  lcd.setCursor(0,3);
  switch (page) {
    case 1:
      lcd.print(F("(Debounce)(LongPres)(Notifica)(  Exit  )"));
      break;
    case 2:
      lcd.print(F("(RingBrig)(RingDim )(AllBrigh)(  Exit  )"));
      break;
    case 3:
      lcd.print(F("                    ( Reboot )(  Exit  )"));
      break;
  }
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
          confMenuAllBrigh();
          break;
        case 3:
          confMenuReboot();
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
    // Button G
    } else if(checkMenuButton(8)){
      switch (page) {
        case 1:
          confMenuReqFm3Scenes();
          break;
      }
      showConfMenu(page);
      checkMenuButtonRelease();
    // Button H
    } else if(checkMenuButton(9)){
      switch (page) {
        case 1:
          page = 2;
          break;
        case 2:
          page = 3;
          break;
        case 3:
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

void printCalibrateExpDown(short confCalibrateExp) {
  if (confCalibrateExp < 10) {
    lcd.setCursor(25,2);
    lcd.print(F("    "));
    lcd.setCursor(24,2);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 100) {
    lcd.setCursor(26,2);
    lcd.print(F("   "));
    lcd.setCursor(24,2);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 1000) {
    lcd.setCursor(27,2);
    lcd.print(F("  "));
    lcd.setCursor(24,2);
    lcd.print(confCalibrateExp);
  } else {
    lcd.print(F(" "));
    lcd.setCursor(24,2);
    lcd.print(confCalibrateExp);
  }
}

void confCalibrateExpDown(byte pedalNumber) {
  short confCalibrateExpDown;
  EEPROM.get(10+(4*pedalNumber),confCalibrateExpDown);
  short nCurrentPotValueTemp = analogRead(pedalNumber);
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Expression Pedal]"));
  lcd.setCursor(0,2);
  lcd.print(F("Expression Pedal  Down: "));
  lcd.setCursor(16,2);
  lcd.print(pedalNumber+1);
  printCalibrateExpDown(confCalibrateExpDown);
  lcd.setCursor(0,3);
  lcd.print(F("(  Save  )                    (  Exit  )"));
  checkMenuButtonRelease();
  while (true) {
    short nCurrentPotValue = analogRead(pedalNumber);
    if(abs(nCurrentPotValue - nCurrentPotValueTemp) > POT_THRESHOLD) {
      confCalibrateExpDown = nCurrentPotValue;
      printCalibrateExpDown(confCalibrateExpDown);
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

void printCalibrateExpUp(short confCalibrateExp) {
  if (confCalibrateExp < 10) {
    lcd.setCursor(23,2);
    lcd.print(F("    "));
    lcd.setCursor(22,2);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 100) {
    lcd.setCursor(24,2);
    lcd.print(F("   "));
    lcd.setCursor(22,2);
    lcd.print(confCalibrateExp);
  } else if (confCalibrateExp < 1000) {
    lcd.setCursor(25,2);
    lcd.print(F("  "));
    lcd.setCursor(22,2);
    lcd.print(confCalibrateExp);
  } else {
    lcd.print(F(" "));
    lcd.setCursor(22,2);
    lcd.print(confCalibrateExp);
  }
}

void confCalibrateExpUp(byte pedalNumber) {
  short confCalibrateExpUp;
  EEPROM.get((10+(4*pedalNumber))+2,confCalibrateExpUp);
  short nCurrentPotValueTemp = analogRead(pedalNumber);
  lcd.setCursor(0,2);
  lcd.print(F("Expression Pedal  Up: "));
  lcd.setCursor(16,2);
  lcd.print(pedalNumber+1);
  printCalibrateExpUp(confCalibrateExpUp);
  checkMenuButtonRelease();
  while (true) {
    short nCurrentPotValue = analogRead(pedalNumber);
    if(abs(nCurrentPotValue - nCurrentPotValueTemp) > POT_THRESHOLD) {
      confCalibrateExpUp = nCurrentPotValue;
      printCalibrateExpUp(confCalibrateExpUp);
      nCurrentPotValueTemp = nCurrentPotValue;
    }
    if(checkMenuButton(save_button)){
      if (expUp[pedalNumber] != confCalibrateExpUp) {
        EEPROM.put((10+(4*pedalNumber))+2,confCalibrateExpUp);
        expUp[pedalNumber] = confCalibrateExpUp;
      }
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
      return;
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printAllBright(byte confAll) {
  lcd.setCursor(16,2);
  lcd.print(F("      "));
  lcd.setCursor(16,2);
  lcd.print(confAll);
}

void confMenuAllBrigh() {
  byte confAllBrigh = EEPROM[30];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Led All Bright]"));
  lcd.setCursor(0,2);
  lcd.print(F("Led All Bright: "));
  printAllBright(confAllBrigh);
  printEditBar();
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(30, confAllBrigh);
      if (confAllBrigh == 0) {
        pixels.setBrightness(0);
      } else {
        pixels.setBrightness((EEPROM[30]*32)-1);
      }
      
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confAllBrigh > 0) {
        confAllBrigh -= 1;
      } else {
        confAllBrigh = 8;
      }
      printAllBright(confAllBrigh);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confAllBrigh < 8) {
        confAllBrigh += 1;
      } else {
        confAllBrigh = 0;
      }
      printAllBright(confAllBrigh);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void confMenuReboot() {
  lcd.clear();
  lcd.setCursor(9,1);
  lcd.print(F("Do you want to reboot?"));
  lcd.setCursor(0,3);
  lcd.print(F("( Reboot )                    (  Exit  )"));
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      actionReboot();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void actionReboot() {
  lcd.clear();
  lcd.setCursor(14,1);
  lcd.print(F("Rebooting..."));
  // Watchdog
  WDT_timings_t config;
  //config.trigger = 5; /* in seconds, 0->128 */
  config.timeout = 1; /* in seconds, 0->128 */
  //config.callback = myCallback;
  wdt.begin(config);
  while (true) {}
}

void printRingBright(byte confRing) {
  lcd.setCursor(17,2);
  lcd.print(F("      "));
  lcd.setCursor(17,2);
  lcd.print(confRing);
}

void confMenuRingBright() {
  byte confRingBright = EEPROM[4];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Led Ring Bright]"));
  lcd.setCursor(0,2);
  lcd.print(F("Led Ring Bright: "));
  printRingBright(confRingBright);
  printEditBar();
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(4, confRingBright);
      ringBright = confRingBright;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confRingBright > 25) {
        confRingBright -= 5;
      } else {
        confRingBright = 100;
      }
      printRingBright(confRingBright);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confRingBright < 100) {
        confRingBright += 5;
      } else {
        confRingBright = 25;
      }
      printRingBright(confRingBright);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printRingDim(byte confRing) {
  lcd.setCursor(14,2);
  lcd.print(F("      "));
  lcd.setCursor(14,2);
  lcd.print(confRing);
}

void confMenuRingDim() {
  byte confRingDim = EEPROM[5];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Led Ring Dim]"));
  lcd.setCursor(0,2);
  lcd.print(F("Led Ring Dim: "));
  printRingDim(confRingDim);
  printEditBar();
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(5, confRingDim);
      ringDim = confRingDim;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confRingDim > 0) {
        confRingDim -= 5;
      } else {
        confRingDim = 50;
      }
      printRingDim(confRingDim);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confRingDim < 50) {
        confRingDim += 5;
      } else {
        confRingDim = 0;
      }
      printRingDim(confRingDim);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printConfReqFm3Scenes(byte confReqFm3Scenes) {
  lcd.setCursor(11,2);
  lcd.print(F("   "));
  lcd.setCursor(11,2);
  switch (confReqFm3Scenes) {
    case 0:
      lcd.print(F("No"));
      break;
    case 1:
      lcd.print(F("Yes"));
      break;
  }
}

void confMenuReqFm3Scenes() {
  byte confReqFm3Scenes = EEPROM[31];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Request FM3 Scenes]"));
  lcd.setCursor(0,2);
  lcd.print(F("Request: "));
  printConfReqFm3Scenes(confReqFm3Scenes);
  printEditBar();
  checkMenuButtonRelease();

  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(31, confReqFm3Scenes);
      requestFm3Scenes = confReqFm3Scenes;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
      return;
    } else if(checkMenuButton(prev_button)){
      if (confReqFm3Scenes == 0) {
        confReqFm3Scenes = 1;
      } else {
        confReqFm3Scenes = 0;
      }
      printConfReqFm3Scenes(confReqFm3Scenes);
      checkMenuButtonRelease();
    } else if(checkMenuButton(next_button)){
      if (confReqFm3Scenes == 0) {
        confReqFm3Scenes = 1;
      } else {
        confReqFm3Scenes = 0;
      }
      printConfReqFm3Scenes(confReqFm3Scenes);
      checkMenuButtonRelease();
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void printOmniPort(byte confOmniPort) {
  lcd.setCursor(11,2);
  lcd.print(F("           "));
  lcd.setCursor(11,2);
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
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Omniport]"));
  lcd.setCursor(0,2);
  lcd.print(F("Omniport : "));
  lcd.setCursor(8,2);
  lcd.print(portNumber+1);
  printOmniPort(confOmniPort);
  printEditBar();
  checkMenuButtonRelease();

  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(6+portNumber, confOmniPort);
      portType[portNumber] = confOmniPort;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
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
  lcd.setCursor(23,2);
  lcd.print(F("        "));
  lcd.setCursor(23,2);
  lcd.print(confNotificationTime * 10);
}

void confMenuNotificationTime() {
  byte confNotificationTime = EEPROM[3];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Notification Time]"));
  lcd.setCursor(0,2);
  lcd.print(F("Notification Time(ms): "));
  printNotificationTime(confNotificationTime);
  printEditBar();
  checkMenuButtonRelease();

  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(3, confNotificationTime);
      notificationTime = confNotificationTime * 10;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
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
  lcd.setCursor(21,2);
  lcd.print(F("       "));
  lcd.setCursor(21,2);
  lcd.print(confLongPressTime * 10);
}

void confMenuLongPressTime() {
  byte confLongPressTime = EEPROM[2];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Long Press Time]"));
  lcd.setCursor(0,2);
  lcd.print(F("Long Press Time(ms): "));
  printLongPressTime(confLongPressTime);
  printEditBar();
  checkMenuButtonRelease();
  
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(2, confLongPressTime);
      longPressTime = confLongPressTime * 10;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
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
  lcd.setCursor(19,2);
  lcd.print(F("     "));
  lcd.setCursor(19,2);
  lcd.print(confDebounceTime * 10);
}

void confMenuDebounceTime() {
  byte confDebounceTime = EEPROM[1];
  lcd.clear();
  lcd.setCursor(0,1);
  lcd.print(F("[Edit Debounce Time]"));
  lcd.setCursor(0,2);
  lcd.print(F("Debounce Time(ms): "));
  printDebounceTime(confDebounceTime);
  printEditBar();
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      EEPROM.update(1, confDebounceTime);
      debounceTime = confDebounceTime * 10;
      printMainMsg(17, F("Saved"), MAIN_MSG_TIME);
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

void printEditBar() {
  lcd.setCursor(0,3);
  lcd.print(F("(  Save  )(  Down  )(   Up   )(  Exit  )"));
}

void printMainMsg(int cursorPos, String lcdText, int delayTime) {
  lcd.clear();
  lcd.setCursor(cursorPos,1);
  lcd.print(lcdText);
  delay(delayTime);
}

uint8_t XORChecksum8(const byte *data, size_t dataLength)
{
  uint8_t value = 0;
  for (size_t i = 0; i < dataLength; i++)
  {
    value ^= (uint8_t)data[i];
  }
  return value&0x7F;
}
