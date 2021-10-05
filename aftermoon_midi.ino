void sendMIDIMessage(byte i, byte actionType) {
  switch (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].type) {
    // Program Change
    case 1:
      MIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      if ((sendMIDIMonitor && editMode) || sendMIDIUSB) {
        usbMIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      }
      break;
    // Control Change
    case 2:
      MIDI.sendControlChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      if ((sendMIDIMonitor && editMode) || sendMIDIUSB) {
        usbMIDI.sendControlChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      }
      break;
    // Midi Clock
    case 7:
      BPM = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0];
      if ((data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]) != 0) {
        int BPMSec = (int)(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
        BPM = (BPMSec << 7) + BPM;
      }
      tempo = (int)(1000/(BPM/60));
      midiClockTempo();
      FM3Tempo();
      HKTempo();
      break;
    // Bank Up
    case 10:
      if (!editMode) {
        hasChangeBank = true;
        pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]) + 1;
        bankUp();
        calculateHaveTempoPreset();
        drawColors();
        lcdChangeBank();
      }
      break;
    // Bank Down
    case 11:
      if (!editMode) {
        hasChangeBank = true;
        pageNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]) + 1;
        bankDown();
        calculateHaveTempoPreset();
        drawColors();
        lcdChangeBank();
      }
      break;
    // Bank Jump
    case 13:
    {
      if (!editMode) {
        hasChangeBank = true;
        byte bankNumberTemp = bankNumber;
        if (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0] == 127) {
          bankNumber = lastBankNumber;
        } else {
          bankNumber = (data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0]);
        }
        lastBankNumber = bankNumberTemp;
        pageNumber = (data.bank[bankNumberTemp-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]) + 1;
        EEPROM[0] = bankNumber;
        calculateHaveTempoPreset();
        drawColors();
        lcdChangeBank();
      }
      break;
    }
    // Toggle Page
    case 14:
      if (!editMode) {
        hasChangeBank = true;
        togglePag();
      }
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
    // FM3 Tuner
    case 21:
    {
      // Enviamos el Tuner al FM3
      byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x11, 0x00 };
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
      break;
    }
    // Delay
    case 23:
      delay((data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0])*10);
      break;
    // Midi Clock Tap
    case 24:
      if (((pressedTime-pressedTapTime) > 2500)) {
        pressedTapTime = pressedTime;
        return;
      }
      if ((pressedTime-pressedTapTime) < 240) {
        BPM = 250;
      } else {
        BPM = (int)(60/((pressedTime-pressedTapTime)/1000.0));
      }
      pressedTapTime = pressedTime;
      tempo = (int)(1000/(BPM/60));
      if (!showBlink) {
        lcdBPM(true, true);
      }
      midiClockTempo();
      FM3Tempo();
      HKTempo();
      break;
    // FM3 Preset Change
    case 25:
      MIDI.sendControlChange(0, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      MIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      if ((sendMIDIMonitor && editMode) || sendMIDIUSB) {
        usbMIDI.sendControlChange(0, data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
        usbMIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[2]);
      }
      isFM3PresetChange = true;
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
      if (actionType == 1 || actionType == 2) {
        dataMiddle[8] = posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posSingle;
      } else {
        dataMiddle[8] = posData.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].posLong;
      }
      
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
      // Recorremos los presets para setear los efectos donde corresponda
      for(byte i_bank=0; i_bank<n_banks; i_bank++){
        for(byte i_page=0; i_page<2; i_page++){
          for(byte i_preset=0; i_preset<n_presets; i_preset++){
            for(byte i_message=0; i_message<n_messages; i_message++){
              if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 26) {
                if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == effectIDIndex) {
                  if (i_bank == (bankNumber-1) && i_page == (pageNumber-1) && i_preset == (buttonNumber-1) && i_message == i) {
                    continue;
                  } else {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 1 || data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 2) {
                      if (dataMiddle[8] == 0) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                      }
                    } else {
                      if (dataMiddle[8] == 0) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                      }
                      
                    }
                  }
                }
              }
            }
          }
        }
      }
      break;
    }
    // FM3 Scene Change
    case 27:
    {
      byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x0C, 0x00 };
      byte numScene = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0];
      dataMiddle[6] = numScene;
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
                if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == numScene) {
                  if (i_bank == (bankNumber-1) && i_page == (pageNumber-1) && i_preset == (buttonNumber-1) && i_message == i) {
                    if (actionType == 1 || actionType == 2) {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                    } else {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                    }
                  } else {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 1 || data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 2) {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                    } else {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 1;
                    }
                  }
                } else {
                  if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 1 || data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 2) {
                    posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                  } else {
                    posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                  }
                }
              }
            }
          }
        }
      }
      fm3PresetChange();
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
      byte numChannel = data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1];
      dataMiddle[8] = numChannel;
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
      // Recorremos los presets para setear los canales donde corresponda
      for(byte i_bank=0; i_bank<n_banks; i_bank++){
        for(byte i_page=0; i_page<2; i_page++){
          for(byte i_preset=0; i_preset<n_presets; i_preset++){
            for(byte i_message=0; i_message<n_messages; i_message++){
              if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 28) {
                if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == effectIDIndex) {
                  if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[1] == numChannel) {
                    if (i_bank == (bankNumber-1) && i_page == (pageNumber-1) && i_preset == (buttonNumber-1) && i_message == i) {
                      if (actionType == 1 || actionType == 2) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                      }
                    } else {
                      if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 1 || data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 2) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 1;
                      }
                    }
                  } else {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 1 || data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action == 2) {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                    } else {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                    }
                  }
                }
              }
            }
          }
        }
      }
      break;
    }
    // Hughes & Kettner Program Change
    case 30:
      MIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      if ((sendMIDIMonitor && editMode) || sendMIDIUSB) {
        usbMIDI.sendProgramChange(data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[0], data.bank[bankNumber-1].page[pageNumber-1].preset[buttonNumber-1].message[i].value[1]);
      }
      HKTempo();
      break;
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

void FM3Tempo() {
  // Enviamos el Tempo al FM3
  if (sendFM3Tempo) {
    byte dataMiddle[] = { 0xF0, 0x00, 0x01, 0x74, 0x11, 0x14, 0x00, 0x00 };
    dataMiddle[6] = (int)BPM&0x7F;
    dataMiddle[7] = (int)BPM >> 7;
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
}

void HKTempo() {
  // Enviamos el Tempo al HK
  if (sendHKTempo) {
    int tempoTemp = (int)(((tempo-50)*255)/1310);
    byte dataMiddle[] = { 0xF0, 0x00, 0x20, 0x44, 0x00, 0x10, 0x00, 0x09, 0x00, 0x04, 0x04, 0x00, 0x00 };
    dataMiddle[11] = tempoTemp >> 7;
    dataMiddle[12] = tempoTemp&0x7F;
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
  delay(10);
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
                  byte actionType = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action;
                  bool effectFind = false;
                  for(byte k=0; k<(z*2); k+=2) {
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == effectIDs[k]) {
                      effectFind = true;
                      if (actionType == 1 || actionType == 2) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingleStatus = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLongStatus = 1;
                      }
                      
                      if ((bitValue(effectIDs[k+1], 0) == 0 && effectType == 26) || ((((effectIDs[k+1] >> 1)&0x03) == data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[1]) && effectType == 28)) {
                        if (actionType == 1 || actionType == 2) {
                          posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                        } else {
                          posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 1;
                        }
                      } else {
                        if (actionType == 1 || actionType == 2) {
                          posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                        } else {
                          posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                        }
                      }
                    }
                  }
                  if (!effectFind && effectType == 26) {
                    if (actionType == 1 || actionType == 2) {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posSingleStatus = 0;
                    } else {
                      posData.bank[i_bank].page[i_page].preset[i_preset].posLongStatus = 0;
                    }
                  }
                }
              }
            }
          }
        }
        if (requestFm3Scenes && isFM3PresetChange) {
          setActualScene = true;
          isFM3PresetChange =  false;
          fm3Scenes(0x7F);
        }
        drawColors();
        delay(200);
        return;
      }
      break;
      // Receive Patch Name
      case 0x0D:
      {
        for(byte i_bank=0; i_bank<n_banks; i_bank++){
          for(byte i_page=0; i_page<2; i_page++){
            for(byte i_preset=0; i_preset<n_presets; i_preset++){
              for(byte i_message=0; i_message<n_messages; i_message++){
                if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].type == 25 &&
                  data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == readData[6] &&
                  data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[1] == readData[7]) {
                  byte actionType = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action;
                  nActualFM3Presets++;
                  data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[3] = 0;
                  char rd2[25];
                  for (int i=0; i<24; i++) {
                      rd2[i] = readData[8+i];
                  }
                  rd2[24] = 0;
                  int bankOffset = (i_bank)*(sizeof(data.bank[i_bank]));
                  int presetOffset;
                  if (i_page == 0) {
                    presetOffset = (sizeof(data.bank[i_bank].bankName)) + ((i_preset)*(sizeof(data.bank[i_bank].page[i_page].preset[i_preset])));
                  } else {
                    presetOffset = (sizeof(data.bank[i_bank].bankName)) + (sizeof(data.bank[i_bank].page[0])) + ((i_preset)*(sizeof(data.bank[i_bank].page[i_page].preset[i_preset])));
                  }
                  // Guardado en memoria
                  strncpy(data.bank[i_bank].page[i_page].preset[i_preset].longName, rd2, 25);
                  // Guardado en EEPROM
                  i2cStat = myEEPROM.write((bankOffset + presetOffset + 1),(byte*)&rd2, 25);
                  if ( i2cStat != 0 ) {
                    printMainMsg(13, F("EEPROM w error"), 0);
                    while (true) {}
                  }
                  rd2[8] = 0;
                  if (actionType == 1 || actionType == 2) {
                    strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, rd2, 9);
                    i2cStat = myEEPROM.write((bankOffset + presetOffset + 26),(byte*)&rd2, 9);
                    if ( i2cStat != 0 ) {
                      printMainMsg(13, F("EEPROM w error"), 0);
                      while (true) {}
                    }
                  } else {
                    strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpShortName, rd2, 9);
                    i2cStat = myEEPROM.write((bankOffset + presetOffset + 44),(byte*)&rd2, 9);
                    if ( i2cStat != 0 ) {
                      printMainMsg(13, F("EEPROM w error"), 0);
                      while (true) {}
                    }
                  }
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
        return;
      }
      break;
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
                    byte actionType = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action;
                    if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == readData[6]) {
                      if (readData[7] == 32) {
                        if (actionType == 1 || actionType == 2) {
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, "Escena ", 9);
                          data.bank[i_bank].page[i_page].preset[i_preset].pShortName[7] = (readData[6]+1) + '0';
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, "Escena ", 9);
                          data.bank[i_bank].page[i_page].preset[i_preset].pToggleName[7] = (readData[6]+1) + '0';
                        } else {
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpShortName, "Escena ", 9);
                          data.bank[i_bank].page[i_page].preset[i_preset].lpShortName[7] = (readData[6]+1) + '0';
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName, "Escena ", 9);
                          data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName[7] = (readData[6]+1) + '0';
                        }
                      } else {
                        char rd2[9];
                        for (int i=0; i<8; i++) {
                            rd2[i] = readData[7+i];
                        }
                        rd2[8] = 0;
                        if (actionType == 1 || actionType == 2) {
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, rd2, 9);
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, rd2, 9);
                        } else {
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpShortName, rd2, 9);
                          strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName, rd2, 9);
                        }
                      }
                      if (actionType == 1 || actionType == 2) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 1;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 1;
                      }
                    } else {
                      if (actionType == 1 || actionType == 2) {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posSingle = 0;
                      } else {
                        posData.bank[i_bank].page[i_page].preset[i_preset].posLong = 0;
                      }
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
                  byte actionType = data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].action;
                  if (data.bank[i_bank].page[i_page].preset[i_preset].message[i_message].value[0] == readData[6]) {
                    if (readData[7] == 32) {
                      if (actionType == 1 || actionType == 2) {
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, "Escena ", 9);
                        data.bank[i_bank].page[i_page].preset[i_preset].pShortName[7] = (readData[6]+1) + '0';
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, "Escena ", 9);
                        data.bank[i_bank].page[i_page].preset[i_preset].pToggleName[7] = (readData[6]+1) + '0';
                      } else {
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpShortName, "Escena ", 9);
                        data.bank[i_bank].page[i_page].preset[i_preset].lpShortName[7] = (readData[6]+1) + '0';
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName, "Escena ", 9);
                        data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName[7] = (readData[6]+1) + '0';
                      }
                      
                    } else {
                      char rd2[9];
                      for (int i=0; i<8; i++) {
                          rd2[i] = readData[7+i];
                      }
                      rd2[8] = 0;
                      if (actionType == 1 || actionType == 2) {
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pShortName, rd2, 9);
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].pToggleName, rd2, 9);
                      } else {
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpShortName, rd2, 9);
                        strncpy(data.bank[i_bank].page[i_page].preset[i_preset].lpToggleName, rd2, 9);
                      }
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
      }
      break;
      default:
      return;
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
    if (readData[1] != 11 && (readData[1] != 12 || (readData[1] == 12 && readData[2] == 1)) && readData[1] != 13 && readData[1] != 14 ) {
      lcd.setCursor(15,2);
      lcd.print(F("Receiving data..."));
    }
  
    switch (readData[1]) {
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
        calculateHaveTempoPreset();
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
        sizeOfData = 13;
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
                receivingDump = true;
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
                  return;
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
                receivingDump = false;
                delay(300);
              }
          } else {
            printMainMsg(7, F("Error al recibir los datos"), MAIN_MSG_TIME);
            return;
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
      // Toggle Preset
      case 15:
        break;
      // Toggle Page
      case 16:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          togglePag();
          sendUSBPresetData();
        }
        break;
      // Bank Down
      case 17:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          pageNumber = 1;
          bankDown();
          sendUSBPresetData();
        }
        break;
      // Bank Up
      case 18:
        sizeOfData = 5;
        if (sizeofsysex == sizeOfData) {
          pageNumber = 1;
          bankUp();
          sendUSBPresetData();
        }
        break;
      // Edit Mode Off
      case 19:
        editMode = false;
        printMainMsg(14, F("EDIT MODE OFF"), MAIN_MSG_TIME);
        lcdChangeAll();
        return;
    }
    lcd.setCursor(15,2);
    lcd.print(F("Success          "));
    delay(50);
    calculateHaveTempoPreset();
    drawColors();
    lcdChangeAll();
  } else {
    if (readData[1] == 8) {
      // Edit Mode On
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
