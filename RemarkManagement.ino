void Remark_Bits() {
  ReadTopway(0x00,0x60);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    if(bitArray[0]==1) {Logout_Timer=0;CheckConfigParam(2);Reset_Rem_Bit();} //1   Add Remark Button
    if(bitArray[1]==1) {Logout_Timer=0;WriteConfigParam(2,"/RMK.csv");Reset_Rem_Bit();} //2   Confirm Button
//    if(bitArray[2]==1) {Logout_Timer=0;DisplayPage(39);Reset_Rem_Bit();} //4     Discard Button
    if(bitArray[3]==1) {Logout_Timer=0;WriteTopway(0x00,0x74,1);Reset_Rem_Bit();} //8     Popup 
    if(bitArray[4]==1) {Logout_Timer=0;Discard_Location(2, "/RMK.csv");Reset_Rem_Bit();} //16   Prev 
//    if(bitArray[6]==1) {Logout_Timer=0;DisplayPage(37);Reset_Rem_Bit();} // 64    Remark Management Add Button

    //Static IP
    if(bitArray[7]==1) {Logout_Timer=0;SaveIPCred();Reset_Rem_Bit();} // 128
    if(bitArray[8]==1) {Logout_Timer=0;WriteTopway(0x00, 0x68, 4);ConnectToWiFi();WriteTopway(0x00, 0x68, 0);DisplayPage(25);Reset_Rem_Bit();} // 128
  }
}

//void CheckRemarkAccess(byte ConfigParam) {
//  if(ConfigParam==1) {DisplayPage(37);}
//  else if(ConfigParam==2) {DisplayPage(39);}
//}

void Reset_Rem_Bit() {
  WriteTopway(0x00, 0x60, 0);
}

void Set_Rem_Bit(byte j, byte val) { 
    ReadTopway(0x00, 0x60); // Read current values into temp_h and temp_l
    if(Read_Err==false) {
      if (j < 8) {
          if (val == 1) {Output_l = temp_l | (1 << j);} else {Output_l = temp_l & ~(1 << j);}
          Output_h = temp_h;
      } else {
          j = j - 8;
          if (val == 1) {Output_h = temp_h | (1 << j);} else {Output_h = temp_h & ~(1 << j);}
          Output_l = temp_l;
      }
      WriteTopway_byte(0x00, 0x60, Output_h, Output_l);
    }
}
