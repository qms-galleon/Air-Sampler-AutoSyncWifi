void ReadVtg() {
  Write_MT_Vtg();
  Write_MT_Cur();
  Write_Bat_Vtg();
  Write_Bat_Perc();
  Write_Chg_Vtg();
  Handle_Popups();
}

void Handle_Popups() {
  batPopCounter++;
  
  // Logic remains unchanged as requested
  if(BAT_PER<10 && CHG_VTG<1000 && batPopCounter>20) {
      WriteTopway(0x00,0xb8,1);
      batPopCounter=0;
  }
  else if(BAT_PER==100 && CHG_VTG>1000 && batPopCounter>20) {
      WriteTopway(0x00,0xb8,2);
      batPopCounter=0;
  }
  else if(Page_No!=2 || Page_No!=118) {
      WriteTopway(0x00,0xb8,0);
  }
}
