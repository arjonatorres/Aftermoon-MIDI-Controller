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
      lcd.print(F("(OmniPor1)(OmniPor2)          (  Page2 )"));
      break;
    case 2:
      lcd.print(F("(Calibr-1)(Calibr-2)          (  Page3 )"));
      break;
    case 3:
      lcd.print(F("(FM3 Menu)                    (  Page1 )"));
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
        case 3:
          showconfMenuFM3();
          checkMenuButtonRelease();
          confMenuFM3();
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

void showconfMenuFM3() {
  lcd.clear();
  // Fila 0 (Superior)
  lcd.setCursor(0,0);
  lcd.print(F("(ReqScene)(ReqPName)                   )"));
  lcd.setCursor(0,1);
  lcd.print(F("[FM3 Configuration Menu]                "));
  // Fila 3 (Inferior)
  lcd.setCursor(0,3);
  lcd.print(F("                              (  Exit  )"));
}

void confMenuFM3() {
  while (true) {
    // Button E
    if(checkMenuButton(6)){
      confmenuReqFm3Scenes();
      showconfMenuFM3();
      checkMenuButtonRelease();
    // Button F
    } else if(checkMenuButton(7)){
      confmenuReqFm3PresetsNames();
      showconfMenuFM3();
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

void confmenuReqFm3Scenes() {
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

void confmenuReqFm3PresetsNames() {
  lcd.clear();
  lcd.setCursor(6,1);
  lcd.print(F("Request all FM3 Preset names?"));
  lcd.setCursor(0,3);
  lcd.print(F("(Request )                    (  Exit  )"));
  checkMenuButtonRelease();
  while (true) {
    if(checkMenuButton(save_button)){
      actionReqFm3PresetsNames();
      checkMenuButtonRelease();
      while (nActualFM3Presets != nTotalFM3Presets) {
        MIDI.read();
      }
      return;
    } else if(checkMenuButton(exit_button)){
      return;
    }
  }
}

void actionReqFm3PresetsNames() {
  // Recorremos los presets para setear los presets
  nActualFM3Presets = 0;
  nTotalFM3Presets = 0;
  for(byte i_bank=0; i_bank<n_banks; i_bank++){
    for(byte i_page=0; i_page<2; i_page++){
      for(byte i_preset=0; i_preset<n_presets; i_preset++){
        for(byte i_message=0; i_message<n_messages; i_message++){
          if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 25) {
            data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[3] = 1;
            nTotalFM3Presets++;
          }
        }
      }
    }
  }

  for(byte i_bank=0; i_bank<n_banks; i_bank++){
    for(byte i_page=0; i_page<2; i_page++){
      for(byte i_preset=0; i_preset<n_presets; i_preset++){
        for(byte i_message=0; i_message<n_messages; i_message++){
          if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 25 &&
            data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[3] == 1) {
            data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[3] = 0;
            lcd.clear();
            lcd.setCursor(14,1);
            lcd.print(F("Synchronize..."));
            // Solicitamos el nombre del preset del FM3
            byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0D, 0x00, 0x00 };
            dataMiddle[6] = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0];
            dataMiddle[7] = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[1];
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
            return;
          }
        }
      }
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
      if (confDebounceTime > 2) {
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
        confDebounceTime = 2;
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
