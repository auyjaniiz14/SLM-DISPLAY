void SOLARMOVE_LABEL(void){
  tft.setCursor(55,10); tft.setTextSize(sizetext); tft.print("S O L A R M O V E");
}

void BATTERY_LABEL(void){
  tft.setCursor(44,65); tft.setTextSize(2); tft.print("BATTERY");
}

void GET_VBATT(void){
  VBATT_TRUE = (VBATT_ADC/VBATT_GAIN)/1000 ;
}

void GET_IBATT(void){
  IBATT_TRUE = (IBATT_ADC/IBATT_GAIN)/1;
  Qin = IBATT_TRUE * 5;
}

void GET_SOLAR_POWER(void){
  SOLAR_POWER_TRUE = VBATT_TRUE * IBATT_TRUE;
  if(SOLAR_POWER_TRUE < 0){
    SOLAR_POWER_TRUE = 0;
  }
}

void GET_LOAD_CURRENT(void){
  LOAD_CURRENT = GET_IBATT_OUT();
  //LOAD_CURRENT = LOAD_CURRENT - 0.4;
  if(LOAD_CURRENT < 0){
    LOAD_CURRENT = 0;
  }
  Qout = LOAD_CURRENT * 5;
}

void GET_LOAD_POWER(void){
  LOAD_POWER_TRUE = (VBATT_TRUE * LOAD_CURRENT) - 4; // Watt true
  if(ChargingState == true){
    //LOAD_POWER_TRUE = LOAD_POWER_TRUE;
    LOAD_POWER_TRUE = LOAD_POWER_TRUE;
  }
  if(LOAD_POWER_TRUE < 0){
    LOAD_POWER_TRUE = 0;
  }
}

void GET_BATT_PERCENT(void){
  if(ChargingState == true){
    eedata = EEPROM.read(eeaddress);
    SOCt0 = eedata / 2.55;
    double Qratio = Qintotal/Qmax;
    int Pert1 = Qratio*100;
    BATT_PERCENT = SOCt0 + Pert1;
    if(BATT_PERCENT > 100){
      BATT_PERCENT = 100;
    }
    else if(BATT_PERCENT < 0){
      BATT_PERCENT = 0;
    }
  }
  if(DischargingState == true && ChargingState == false){
    eedata = EEPROM.read(eeaddress);
    SOCt0 = eedata / 2.55;
    double Qratio = Qouttotal/Qmax;
    int Pert1 = Qratio*100;
    BATT_PERCENT = SOCt0 - Pert1;
    if(BATT_PERCENT > 100){
      BATT_PERCENT = 100;
    }
    else if(BATT_PERCENT < 0){
      BATT_PERCENT = 0;
    }
  }
  if(ChargingState == false && DischargingState == false){
    eedata = EEPROM.read(eeaddress);
    SOCt0 = (eedata / 2.55) + 1;
    BATT_PERCENT = SOCt0;
    int BATT_PERCENT2 = (VBATT_TRUE - 10) / 0.027;
    if(SOCt0 > BATT_PERCENT2){
      BATT_PERCENT = SOCt0;
      if(BATT_PERCENT > 100){
        BATT_PERCENT = 100;
      }
      else if(BATT_PERCENT < 0){
        BATT_PERCENT = 0;
      }
    }
    else if(SOCt0 <= BATT_PERCENT2){
      BATT_PERCENT = BATT_PERCENT2;
      if(BATT_PERCENT > 100){
        BATT_PERCENT = 100;
      }
      else if(BATT_PERCENT < 0){
        BATT_PERCENT = 0;
      }
      int EEinput = BATT_PERCENT*2.55;
      EEPROM.put(eeaddress,EEinput);
    }
  }
}

void GET_VAL(void){
  SPLIT_STRING();
}

void SPLIT_STRING(void){
  int count = 0;
    while(count < 6){
    ADC_STRING_IN = Serial.readStringUntil('\n');
    if(ADC_STRING_IN.substring(1,6) == "Vbatt" ){
      VBATT_ADC_STRING = ADC_STRING_IN.substring(8,15);
      RawVBATT_ADC = VBATT_ADC_STRING.toInt();
      VBATT_ADC += RawVBATT_ADC;
    }
    if(ADC_STRING_IN.substring(1,6) == "Ibatt"){
      IBATT_ADC_STRING = ADC_STRING_IN.substring(8,15);
      RawIBATT_ADC = abs(IBATT_ADC_STRING.toInt());
      IBATT_ADC += RawIBATT_ADC;
      Qintotal += Qin;
      Qouttotal += Qout;
    }
    if(ADC_STRING_IN.substring(1,7) == "Vsolar"){
      VSOLAR_ADC_STRING = ADC_STRING_IN.substring(9,16);
      RawVSOLAR_ADC = VSOLAR_ADC_STRING.toInt();
      VSOLAR_ADC += RawVSOLAR_ADC;
      count++;
    }
    if(count == 5){
        VBATT_ADC = VBATT_ADC/6; IBATT_ADC = IBATT_ADC/6; VSOLAR_ADC = VSOLAR_ADC/6;
        GET_VBATT();
        GET_LOAD_CURRENT();
        if(LOAD_CURRENT > 1.5){
          DischargingState = true;
        }
        if(LOAD_CURRENT < 1.5){
          DischargingState = false;
        }
        if(IBATT_ADC > 10 && VSOLAR_ADC > 150){
          ChargingState = true;
        }
        if(IBATT_ADC < 10 && VSOLAR_ADC > 150){
          ChargingState = false;
        }
        if(IBATT_ADC < 10 && VSOLAR_ADC < 150){
          ChargingState = false;
        }
        GET_IBATT();
        GET_BATT_PERCENT();
        GET_SOLAR_POWER(); GET_LOAD_POWER();
        return count = 0;
     }
   }
}

void LCD_OUT(void){
  BATT_VOLT_LCD();
  BATT_PERCENT_LCD();
  PV_WATT_LCD();
  LOAD_WATT_LCD();
}

void BATT_VOLT_LCD(void) {
  char BUFF_LCD[5];
  tft.setCursor(50,185);
  sprintf(BUFF_LCD,"%d.%01d V",int(VBATT_TRUE),((int)(fabsf(VBATT_TRUE)*10)%10));
  tft.setTextColor(WHITE, BLACK);
  tft.print(BUFF_LCD);
}

void BATT_PERCENT_LCD(void){
  char BUFF_LCD[5];
  tft.setTextSize(2);
  tft.setCursor(55,128);
  sprintf(BUFF_LCD, "%3d", BATT_PERCENT);
  tft.setTextColor(WHITE, BLACK);
  tft.print(BUFF_LCD);
  tft.setCursor(100,128);
  tft.setTextColor(WHITE, BLACK);
  tft.print("%");
  DRAW_CIRCLE_CL();
}

void DRAW_CIRCLE_CL(void){
  for(int i=36;i<43;i++){
    tft.drawCircle(85,134,i,WHITE);
  }
}

void PV_WATT_LCD(void){
  char BUFF_LCD[5];
  tft.setTextSize(3);
  tft.setCursor(235, 50);
  sprintf(BUFF_LCD, "%3d", SOLAR_POWER_TRUE);
  tft.setTextColor(WHITE, BLACK);
  tft.print(BUFF_LCD);
  tft.setCursor(295, 50);
  tft.setTextColor(WHITE, BLACK);
  tft.print("W");
}

void LOAD_WATT_LCD(void){
  char BUFF_LCD[5];
  tft.setTextSize(3);
  tft.setCursor(235, 113);
  sprintf(BUFF_LCD, "%3d", LOAD_POWER_TRUE);
  tft.setTextColor(WHITE, BLACK);
  tft.print(BUFF_LCD);
  tft.setCursor(295, 113);
  tft.setTextColor(WHITE, BLACK);
  tft.print("W");
}

double GET_IBATT_OUT(){
  for(int i=0;i<100;i++){
    RawVoltage = analogRead(A5);
    istVoltage += RawVoltage;
    delayMicroseconds(5);
  }
  istVoltage = istVoltage/100;
  double v = (istVoltage / 1023.0) * 5000;
  double c = ((v - 2525)/100) - 0.2; // 2525
  if(c < 0){
    c = 0;
  }
  return c;
  return istVoltage = 0;
  return v = 0;
}
