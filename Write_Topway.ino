void WriteTopway(byte adr_h, byte adr_l,int writeback) {
  writeback_h = (writeback >> 8) & 0xFF;
  writeback_l = writeback & 0xFF;
  WriteTopway_byte(adr_h,adr_l,writeback_h,writeback_l);
}

void WriteTopway_byte(byte adr_h, byte adr_l,byte writeback_h,byte writeback_l) {
  Serial.write(0xaa);//packet head
  Serial.write(0x3d);// VP_N16 write command
  Serial.write((byte) 0x00); // VP_N16 address
  Serial.write((byte) 0x08);
  Serial.write((byte) adr_h);
  Serial.write((byte) adr_l);
  Serial.write(writeback_h);  // VP_N16 data high byte
  Serial.write(writeback_l);  // VP_N16 data low byte
  EndPacket();
}

void WriteTopway_32(byte adr_h, byte adr_l,long writeback) {
  writeback_h = (writeback >> 24) & 0xFF;
  writeback_m1 = (writeback >> 16) & 0xFF;
  writeback_m2 = (writeback >> 8) & 0xFF;
  writeback_l = writeback & 0xFF;
  WriteTopway_byte_32(adr_h,adr_l,writeback_h,writeback_m1, writeback_m2, writeback_l);
}

void WriteTopway_byte_32(byte adr_h,byte adr_l,byte writeback_h,byte writeback_m1,byte writeback_m2,byte writeback_l) {
  Serial.write(0xaa);//packet head
  Serial.write(0x44);// VP_N16 write command
  Serial.write((byte) 0x00); // VP_N16 address
  Serial.write((byte) 0x02);
  Serial.write((byte) adr_h);
  Serial.write((byte) adr_l);
  Serial.write(writeback_h);  // VP_N32 data high byte
  Serial.write(writeback_m1);  // VP_N32 data m1 byte
  Serial.write(writeback_m2);  // VP_N32 data m2 byte
  Serial.write(writeback_l);  // VP_N32 data low byte
  EndPacket();
}

void WriteString(byte adr_h1, byte adr_m1, byte adr_l1, String StringTemp1) {
  
  char buffer1[StringTemp1.length() + 1];
  StringTemp1.toCharArray(buffer1, sizeof(buffer1));
  
  Serial.write(0xaa);//packet head
  Serial.write(0x42);// String write command
  Serial.write((byte) 0x00); // String address
  Serial.write((byte) adr_h1);
  Serial.write((byte) adr_m1);
  Serial.write((byte) adr_l1);
  Serial.write(buffer1);
  Serial.write((byte) 0x00);
  EndPacket();
}

void Write_Count_Down_Perc() {
  Motor_Counter_Perc=map(Motor_Counter,0,Motor_Counter_Max,180,0);
  Motor_Counter_Perc=constrain(Motor_Counter_Perc,0,180);
  writeback_h=0;writeback_l=Motor_Counter_Perc;
  WriteTopway(0x00,0x30,Motor_Counter_Perc);
}

void Write_Count_Down() {
  WriteTopway(0x00,0x2E,Motor_Counter);
}

void Write_Motor_PWM1() {
  WriteTopway(0x00,0x00,Motor_PWM[0]);
}

//************************Write Clock*******************************
void WriteTime() {
  String StrTemp;
  StrTemp=daysOfTheWeek[now1.dayOfTheWeek()];
  Str1=StrTemp+" ";
  if(now1.day()<10) {Str1+='0';}
  Str1=Str1+now1.day()+"/";
  if(now1.month()<10) {Str1+='0';}
  Str1=Str1+now1.month()+"/"+now1.year()+" ";
  
  if(now1.hour()<10) {Str1+='0';}
  Str1=Str1+now1.hour()+":";
  if(now1.minute()<10) {Str1+='0';}
  Str1=Str1+now1.minute()+":";
  if(now1.second()<10) {Str1+='0';}
  Str1=Str1+now1.second();
  WriteString(0x00,0x02,0x80,Str1);
}

void updateClock() {
  ReadTopway(0x00,0x3C);if(Read_Err==false) {date_set=temp_l;} //else {goto Skip;}
  ReadTopway(0x00,0x3E);if(Read_Err==false) {month_set=temp_l;}
  ReadTopway(0x00,0x40);if(Read_Err==false) {year_set=(temp_h << 8) | temp_l;}
  ReadTopway(0x00,0x42);if(Read_Err==false) {hour_set=temp_l;}
  ReadTopway(0x00,0x44);if(Read_Err==false) {minute_set=temp_l;}
  rtc.adjust(DateTime(year_set, month_set, date_set, hour_set, minute_set, 0));
  delay(10);
  now1 = rtc.now();
  if(date_set==now1.day() && month_set==now1.month() && year_set==now1.year() && hour_set==now1.hour() && minute_set==now1.minute()) {
    WriteTopway(0x00,0x80,1);
  } else {
    WriteTopway(0x00,0x80,2);
  }
  Set_In_Bit(13, 0);
  
//  Skip:;
}
//************************End Clock*******************************

//************************Write Motor Parameters*******************************
void Write_MT_Vtg() {
  WriteTopway(0x00,0x18,MTR_VTG);
}

void Write_MT_Cur() {
//  WriteTopway(0x00,0x20,ErrorCounter);
  WriteTopway(0x00,0x20,MTR_CUR);
}

void Write_Bat_Vtg() {
  WriteTopway(0x00,0x28,BAT_VTG);
}

void Write_Bat_Perc() {
  WriteTopway(0x00,0x2a,BAT_PER);
}

void Write_Chg_Vtg() {
  WriteTopway(0x00,0x2c,CHG_VTG);
  // LogActivity is safe here as it was updated in the previous file
  if(CHG_VTG>1000 && ChargerStatus==false) {ChargerStatus=true;Set_Bit_Icons(0,1);LogActivity("Charger Connected");/*LogBatParameters();*/} 
  if(CHG_VTG<1000 && ChargerStatus==true) {ChargerStatus=false;Set_Bit_Icons(0,0);LogActivity("Charger Disconnected");/*LogBatParameters();*/}
}
//************************End Motor Parameters*******************************


void DisplayPage(int i) {
  Serial.write(0xaa);//packet head
  Serial.write(0x70);// Page write command
  Serial.write((byte) 0x00);
  Serial.write(i); // Page No
  EndPacket();
}

void EndPacket() {
  Serial.write(0xcc);
  Serial.write(0x33);
  Serial.write(0xc3);
  Serial.write(0x3c);
}

void SetFG(int PageNo, int strID, int ColorID) { //0 - Black, 1 - White, 2 - Red, 3 - Green, 4 - Orange
  if(Page_No==104 || Page_No==105 || Page_No==137) {
    FGCmd();
    Serial.write((byte) PageNo);
    Serial.write((byte) strID);
    if(ColorID==0) {Serial.write((byte) 0x00);Serial.write((byte) 0xff);Serial.write((byte) 0xff);}
    else if(ColorID==1) {Serial.write((byte) 0x00);Serial.write((byte) 0x00);Serial.write((byte) 0x00);}
    else if(ColorID==2) {Serial.write((byte) 0x00);Serial.write((byte) 0xf0);Serial.write((byte) 0x00);}
    else if(ColorID==3) {Serial.write((byte) 0x00);Serial.write((byte) 0x0f);Serial.write((byte) 0x00);}
    else if(ColorID==4) {Serial.write((byte) 0x00);Serial.write((byte) 0xff);Serial.write((byte) 0x00);}
    EndPacket();
  }
}

void FGCmd() {
  Serial.write(0xaa);
  Serial.write(0x7e);// VP_N16 write command
  Serial.write((byte) 0x01);
  Serial.write((byte) 0x00);
}
