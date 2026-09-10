void QuickSample() {
  ReadTopway(0x00,0x86);
  if(Read_Err==false) {
    for (i = 7; i >= 0; i--) {
      bitArray[i] = (temp_l >> i) & 1;  // Extract each bit from the low byte
    }
    for (int i = 7; i >= 0; i--) {
      bitArray[i + 8] = (temp_h >> i) & 1;  // Extract each bit from the high byte
    }
    // User Last Sample
    if(bitArray[0]==1) {
        if(BAT_PER>20) {
            if(CalStatus==true) {
                Logout_Timer=0;
                WriteLastUserSample();
            } else {
                WriteTopway(0x00,0x98,2);
            }
        } else {
            WriteTopway(0x00,0xb8,3);
        } 
        Reset_Quick();
    } 
    
    // Device Last Sample
    if(bitArray[1]==1) {
        if(BAT_PER>20) {
            if(CalStatus==true) {
                Logout_Timer=0;
                WriteLastDeviceSample();
            } else {
                WriteTopway(0x00,0x98,2);
            }
        } else {
            WriteTopway(0x00,0xb8,3);
        } 
        Reset_Quick();
    } 
    
    // Recipe Sample
    if(bitArray[2]==1) {
        if(BAT_PER>20) {
            if(CalStatus==true) {
                Logout_Timer=0;
                DisplayPage(80);
            } else {
                WriteTopway(0x00,0x98,2);
            }
        } else {
            WriteTopway(0x00,0xb8,3);
        } 
        Reset_Quick();
    } 
    
    // Sample Run
    if(bitArray[3]==1) {
        if(BAT_PER>20) {
            if(CalStatus==true) {
                Logout_Timer=0;
                DisplayPage(4);
            } else {
                WriteTopway(0x00,0x98,2);
            }
        } else {
            WriteTopway(0x00,0xb8,3);
        } 
        Reset_Quick();
    } 
  }
}

void WriteLastDeviceSample() {
  WriteString(0x00,0x03,0x00,LastSampleMode);
  WriteString(0x00,0x04,0x80,LastSampleLocation);
  WriteTopway(0x00,0x08,LastVolume);
  WriteTopway(0x00,0x34,LastStartDelay);
  WriteTopway(0x00,0x36,LastNoofSamples);
  WriteTopway(0x00,0x38,LastDelaybetwRuns);
  DisplayPage(7);
}

void WriteLastUserSample() {
  WriteString(0x00,0x03,0x00,UserLastSampleMode);
  WriteString(0x00,0x04,0x80,UserLastSampleLocation);
  WriteTopway(0x00,0x08,UserLastVolume);
  WriteTopway(0x00,0x34,UserLastStartDelay);
  WriteTopway(0x00,0x36,UserLastNoofSamples);
  WriteTopway(0x00,0x38,UserLastDelaybetwRuns);
  DisplayPage(7);
}

void WriteOnlineSample() {
  WriteString(0x00,0x03,0x00,Mode);
  WriteString(0x00,0x01,0x80,SampleName);
  WriteString(0x00,0x04,0x80,SamplingLocation);
  WriteTopway(0x00,0x08,Motor_lts);
  WriteTopway(0x00,0x34,Start_Delay1);
  WriteTopway(0x00,0x36,No_of_Runs);
  WriteTopway(0x00,0x38,Delay_between_Runs);
}

void Reset_Quick() {
  WriteTopway(0x00, 0x86, 0);
}
