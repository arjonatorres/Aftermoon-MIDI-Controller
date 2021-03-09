#include <Adafruit_NeoPixel.h>
#include <MIDI.h>
#include <EEPROM.h>
#include <Wire.h> 
#include <LiquidCrystal_I2C_40x4.h>
#include <extEEPROM.h>
#include "Watchdog_t4.h"

#define n_messages 8
#define n_presets 8
#define n_banks 20
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
boolean hasChangeBank = false;
boolean isFM3PresetChange = false;
int nActualFM3Presets;
int nTotalFM3Presets;
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
extEEPROM myEEPROM(kbits_512, 1, 64);
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
  
  MIDI.begin(MIDI_CHANNEL_OMNI);
  MIDI.turnThruOff();
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
  MIDI.setHandleSystemExclusive(OnSysEx);
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
  MIDI.read();
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
          if (lpColorType == 0 || posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posLongStatus == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          } else {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringDim)/100))), (uint8_t)(round(g*(((float)ringDim)/100))), (uint8_t)(round(b*(((float)ringDim)/100)))));
            }
          }
        } else {
          if (posData.bank[bankNumber-1].page[pageNumber-1].preset[i].posLongStatus == 0) {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color(0,0,0));
            }
          } else {
            for(int j=4; j<7; j++){
              pixels.setPixelColor((i*8)+j, pixels.Color((uint8_t)(round(r*(((float)ringBright)/100))), (uint8_t)(round(g*(((float)ringBright)/100))), (uint8_t)(round(b*(((float)ringBright)/100)))));
            }
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
          if (!editMode) {
            pageNumber = 1;
            bankDown();
            drawColors();
            lcdChangeBank();
          } else {
            printMainMsg(10, F("EXIT EDIT MODE FIRST"), MAIN_MSG_TIME);
            lcdChangeAll();
          }
          checkMenuButtonRelease();
        // B+C check (Toggle Page)
        } else if (digitalRead(3)== HIGH && digitalRead(4)== HIGH) {
          if (!editMode) {
            togglePag();
            drawColors();
          } else {
            printMainMsg(10, F("EXIT EDIT MODE FIRST"), MAIN_MSG_TIME);
            lcdChangeAll();
          }
          checkMenuButtonRelease();
        // C+D check (Bank Up)
        } else if (digitalRead(4)== HIGH && digitalRead(5)== HIGH) {
          if (!editMode) {
            pageNumber = 1;
            bankUp();
            drawColors();
            lcdChangeBank();
          } else {
            printMainMsg(10, F("EXIT EDIT MODE FIRST"), MAIN_MSG_TIME);
            lcdChangeAll();
          }
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
          if (hasChangeBank) {
            hasChangeBank = false;
          } else {
            // Trigger Release All Action
            checkMsg(9);
            if (hasChangeBank){
              hasChangeBank = false;
            }
          }
          drawColors();
          lcdPresetNames();
          checkMenuButtonRelease();
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
    byte buttonNumberTemp;
    if (buttonNumber <= 4) {
      buttonNumberTemp = buttonNumber-1;
      lcd.setCursor(((buttonNumberTemp)*10),3);
    } else {
      buttonNumberTemp = buttonNumber-5;
      lcd.setCursor(((buttonNumberTemp)*10),0);
    }
    if (posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong == 0 || (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 6) == 0) ) {
      printPresetPos(buttonNumberTemp+1, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].lpShortName);
    } else {
      printPresetPos(buttonNumberTemp+1, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].lpToggleName);
    }
  }
  
  while (true) {
    // Long Press
    if (((millis() - pressedTime) > longPressTime) && haveLPMsg) {
      // Trigger Long Press Action
      checkMsg(3);

      if (hasChangeBank){
        return;
      }
      
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
      if (hasChangeBank){
        return;
      }
      if (bitValue(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].presetConf, 0) == 1) {
        presetBankNumber = bankNumber;
        presetPageNumber = pageNumber;
        presetButtonNumber = buttonNumber;
        presetSPNumber = 0;
        lcdPreset();
      }
      
      // Trigger Release Action
      checkMsg(2);
      if (hasChangeBank){
        return;
      }
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
            sendMIDIMessage(i, action);
          }
          break;
        case 3:
        case 4:
          if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong ||
              data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].pos == 2) {
            sendMIDIMessage(i, action);
          }
          break;
        case 9:
          sendMIDIMessage(i, action);
          break;
      }
    }
  }
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
}

void bankUp() {
  if (bankNumber == n_banks) {
    bankNumber = 1;
  } else {
    bankNumber++;
  }
  EEPROM[0] = bankNumber;
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
