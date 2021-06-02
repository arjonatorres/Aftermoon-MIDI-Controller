void lcdChangeAll() {
  lcd.clear();
  lcdPresetNames();
  lcdBank();
  lcdPreset();
  if (!showBlink) {
    lcdBPM(true, true);
  }
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

void lcdChangeBank() {
  if (!editMode && notificationTime != 0) {
    lcd.clear();
    lcd.setCursor(((40-strlen(data.bank[bankNumber-1].bankName))/2)-1,1);
    lcd.print(F(data.bank[bankNumber-1].bankName));
    lcd.setCursor(16,2);
    lcd.print(F("Bank "));
    lcd.setCursor(21,2);
    lcd.print(bankNumber, DEC);
    delay(notificationTime);
    lcd.clear();
  }
  lcdPresetNames();
  lcdBank();
}

void lcdPreset() {
  lcd.setCursor(0,2);
  if (!editMode) {
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
  } else {
    lcd.print(F("[In Edit Mode]                   "));
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

void printMainMsg(int cursorPos, String lcdText, int delayTime) {
  lcd.clear();
  lcd.setCursor(cursorPos,1);
  lcd.print(lcdText);
  delay(delayTime);
}

void lcdBPM(boolean activo, boolean mostrar) {
  if (showBlink || mostrar) {
    if (activo) {
      if (BPM < 100) {
        lcd.setCursor(33,2);
        midiClockTempo();
        lcd.print(F(" "));
        midiClockTempo();
        lcd.setCursor(34,2);
      } else {
        lcd.setCursor(33,2);
      }
      midiClockTempo();
      lcd.print((int)BPM, DEC);
      midiClockTempo();
      lcd.setCursor(37,2);
      midiClockTempo();
      lcd.print(F("BPM"));
      midiClockTempo();
    } else {
      lcd.setCursor(33,2);
      midiClockTempo();
      lcd.print(F("       "));
      midiClockTempo();
    }
  }
}
