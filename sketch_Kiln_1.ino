// 
// Modified by Jason Fowler 5/17/2014.
// Copyright 2014 - Attribution-ShareAlike 4.0 International
// Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
//
// Icons: www.visualpharm.com  creativecommons.org/licenses/by-nd/3.0
// This software is furnished "as is", without technical support, and with no 
// warranty, express or implied, as to its usefulness for any purpose.
//

#include <ezLCDLib.h>
#include "Adafruit_MAX31855.h"

// Variables
ezLCD3 lcd; // main LCD interface
Adafruit_MAX31855 thermocouple(5, 4, 3); //Pins 5,4,3

volatile boolean ezLCDInt = false; // flag to indicate interrupt
byte screen = 1; // Screen index
int probedisconnect = 0; //Probe disconnect tracking

// Used to store Firing Schedue
int schRate[10];
int schTemp[10];
int schSoak[10];
char schName[33];
int schLastStep = 0;
int iStep = 0;
int rampStart = 0;
int soakMinute = 0;

//Control Temp Variables
int currentTemp = 0;
int startTemp = 0;
unsigned long previousMillis = 0;
int seconds = 0;
int minutes = 0;
boolean reachTemp = 0;
boolean schRunning = 0;

// Program setup 
void setup()
{
  delay(500); // delay for thermocouple
  pinMode(7,OUTPUT); // Kiln Control

  lcd.begin( EZM_BAUD_RATE );
  lcd.sendCommand("cfgio 9 touch_int low quiet");
  lcd.sendCommand("Threshold 200"); // Touchscreen 10-256 hard - soft 

  ezLCDInt = false;
  attachInterrupt(0, ezLCDevent, RISING ); 

  delay(2000); // Startup delay to allow splash screen viewing

  // arLCD Themes
  lcd.theme(2, 1, 2, 3, 3, 3, 111, 105, 5, 0, 0); // Preset Screen Buttons

  // Widgets and Screens
  /* Screen 1 = Preset Fire Schedule Selection
   1-9 string
   1-9 widgets
   2 Theme IDs
   Screen 2 = Load Preset & Verification
   20-21 widgets
   Screen 3 = Run Schedule
   31-32 string
   31-36 widgets
   7,10,11,12 Theme IDs
   */

  // Show first screen - Preset Selection
  showPresetScreen();

}// End setup()

// Main program loop
void loop()
{
  // Always track current seconds and minutes
  // Get current millis
  unsigned long currentMillis = millis();

  //Count Seconds
  if(currentMillis - previousMillis >= 1000) {// 1 second has passed
    // Save to previous millis to count seconds 
    previousMillis = currentMillis;
    seconds++; //Increment seconds

    //Count Minutes
    if (seconds >= 60){// 1 minute has passed
      seconds = 0; //Reset Minute counter
      minutes++;
    }//end if minute
  }//end if second

  //-----------------------------------
  // Main screen input handeling
  if(  ezLCDInt ) // Check in LCD input Interrupt
  {
    ezLCDInt = false;
    lcd.wstack(0);

    switch (screen) { //Find out what Screen we are on
    case 1:
      processPresetScreen();
      break;
    case 2:
      processDisplaySchedule();
      break;
    case 3:
      processRunScreen();
      break;
    case 4:
      processDisplayDisconnect();
      break;
    default:
      break; 
      // if nothing else matches, do the default
      // default is optional
    }//End Switch Case
  }// End if(  ezLCDInt ) Screen Touch

  //-----------------------------------
  // Probe disconnect protection
  if (currentTemp <= 40 && schRunning == 1){
    probedisconnect++;
    if (probedisconnect > 100){
      schRunning = 0; // Set Schedule Running to 0 to break out
      DisplayDisconnect();
    }
  }
  else{
    probedisconnect = 0;
  }   

  //-----------------------------------
  // Main firing schedule loop
  if ( iStep <= schLastStep && schRunning == 1 ){// not end of firing schedule
     // Get current Temp
    currentTemp = getTemp();
    lcd.wvalue(31, currentTemp); // Update Meter

    // Get Ramp Schedule Temp
    int minRunning = (minutes - rampStart);
    int rampTemp = ((schRate[iStep] / 60) * minRunning + startTemp);

    lcd.wvalue(40, seconds); //xxx
    lcd.wvalue(41, minRunning); //xxx
    lcd.wvalue(42, rampTemp ); //xxx
    lcd.wvalue(43, 0); //xxx

    // Check if target temp reached
    if (currentTemp >= schTemp[iStep] && reachTemp == 0){
      reachTemp = 1;
      tempLED(1);
      soakMinute = minutes; // Set soak start minute
    } 

    if (reachTemp){
      // Stay at this Temp for the soak period
      soakLED(1);

      //Check if 1 minute has passed
      int iPassed = minutes - soakMinute;
      if (iPassed >= 1){// 1 minute has passed
        soakMinute = minutes; // set for next minute

        //Update Soak Time
        schSoak[iStep] = schSoak[iStep] -1;
        lcd.wvalue(34, schSoak[iStep]);
      }//end if minute

      // Check if the Soak time has passed
      if (schSoak[iStep] <= 0){// Go to the next step
        iStep++;
        //Check if we are NOT done with the schedue
        if (iStep <= schLastStep){// Next Step
          displayStepInfo();
        }
      } //End If Soak over

      // maintain target temp for soak
      if (currentTemp >= schTemp[iStep]){
        kilnLED(0);
      }
      else{
        kilnLED(1); 
      }

    }//End (reachTemp)
    else{// keep working ramp to target temp
      if (currentTemp >= rampTemp){
        kilnLED(0);
      }
      else{
        kilnLED(1); 
      }
    }//End Ramp Temp Check
  }// End if NOT end of schedule
  else if (screen == 3){//End of Schedule
    kilnLED(0); 
    soakLED(0);
    tempLED(0);

    // Get current Temp
    currentTemp = getTemp();
    lcd.wvalue(31, currentTemp); // Update Meter
  }  

}// End loop()

void ezLCDevent( void ) {
  ezLCDInt = true;
}

void showPresetScreen()
{
  screen = 1; // Set Screen ID for Input  
  lcd.font(0);
  lcd.cls( BLACK, WHITE );
  lcd.xy(0,0);
  lcd.print("Select Preset Firing Schedule:");

  lcd.string(1,"Preset 1");
  lcd.string(2,"Preset 2");
  lcd.string(3,"Preset 3");
  lcd.string(4,"Preset 4");
  lcd.string(5,"Preset 5");
  lcd.string(6,"Preset 6");
  lcd.string(7,"Preset 7");
  lcd.string(8,"Preset 8");
  lcd.string(9,"Preset 9");

  lcd.button( 1, 0, 30, 100, 50, 1, 0, 2, 2, 1 );
  lcd.button( 2, 110, 30, 100, 50, 1, 0, 2, 2, 2 );
  lcd.button( 3, 220, 30, 100, 50, 1, 0, 2, 2, 3 );
  lcd.button( 4, 0, 90, 100, 50, 1, 0, 2, 2, 4 );
  lcd.button( 5, 110, 90, 100, 50, 1, 0, 2, 2, 5 );
  lcd.button( 6, 220, 90, 100, 50, 1, 0, 2, 2, 6 );
  lcd.button( 7, 0, 150, 100, 50, 1, 0, 2, 2, 7 );
  lcd.button( 8, 110, 150, 100, 50, 1, 0, 2, 2, 8 );
  lcd.button( 9, 220, 150, 100, 50, 1, 0, 2, 2, 9 );
}

//Process the Preset Screen Selection
void processPresetScreen()
{
  //  if (( lcd.currentWidget == 1 ) && ( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if Button 1 
  //    char fn[16] = "preset1.txt";
  //    ReadFile(fn);
  //  }
  if (( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if Button 1 
    switch (lcd.currentWidget) {
    case 1:
      {
        char fn[16] = "preset1.txt";
        ReadFile(fn);
        break;
      }
    case 2:
      {
        char fn[16] = "preset2.txt";
        ReadFile(fn);
        break;
      }
    case 3:
      {
        char fn[16] = "preset3.txt";
        ReadFile(fn);
        break;
      }
    case 4:
      {
        char fn[16] = "preset4.txt";
        ReadFile(fn);
        break;
      }
    case 5:
      {
        char fn[16] = "preset5.txt";
        ReadFile(fn);
        break;
      }
    case 6:
      {
        char fn[16] = "preset6.txt";
        ReadFile(fn);
        break;
      }
    case 7:
      {
        char fn[16] = "preset7.txt";
        ReadFile(fn);
        break;
      }
    case 8:
      {
        char fn[16] = "preset8.txt";
        ReadFile(fn);
        break;
      }
    case 9:
      {
        char fn[16] = "preset9.txt";
        ReadFile(fn);
        break;
      }
    default: 
      break;
      // if nothing else matches, do the default
      // default is optional

    }//End Switch Case
  }//End IF
}//End processPresetScreen()

// Read Firing Schedule from arLCD Flash memory into buffer
void ReadFile(char fileName[16])
{
  char buffer [256];
  int fsReturn, bcount;

  //Prep screen
  lcd.font( 1 ); // Use small font
  lcd.cls( BLACK, WHITE ); // clear screen to white
  lcd.print( "Loading Preset..." );

  //Change to root folder on arLCD
  fsReturn = lcd.FSchdir( "\\\\" );//change to root
  fsReturn = lcd.FSchdir( "Schedule" );
  if ( fsReturn == 0 ) {
    lcd.print( " OK" );
    fsReturn = lcd.FSopen( fileName, "r" ); //Open File

      if ( fsReturn == 0 ){
      lcd.print( " OK\r" );
      fsReturn = lcd.FSread( buffer, 256 ); //Read File
      //lcd.print( buffer );//DEBUG
      ReadSchedule(buffer); // GOTO next function and process the data
    }//End if
    else{
      lcd.print( "\rFailed to open file" );
      lcd.print( "\rReset in 3 Seconds!");
      delay(3000);
      showPresetScreen();
    }
    //Close File
    fsReturn = lcd.FSclose();     
  }//End If
  else{
    lcd.print( "\rSchedules are not present" );
  }
}//End Read File Function

// Read the schedule buffer into memory  
void ReadSchedule(char buffer [256])
{  
  //Convert buffer to String object for processing 
  String  message(buffer); // holds text not yet split
  int     stepPosition;  // the position of the next comma in the string
  iStep = 0; // Reset schedule step

  do{//Break message into schedule steps based on the ;
    stepPosition = message.indexOf(';');

    if(stepPosition != -1){
      String schStep = message.substring(0,stepPosition);
      int commaPosition;    // the position of the next comma in the string 
      int schTmp[4];        // temp memory to hold schedule step information
      int tmpIndex = 0;     // schedule temp memory index

      do{//Break the step into Rate,Temp,Soak
        commaPosition = schStep.indexOf(',');

        if(commaPosition != -1){
          // Split out , and temp store
          String stepPart = schStep.substring(0,stepPosition);
          schTmp[tmpIndex] = stepPart.toInt();
          tmpIndex++;
          schStep = schStep.substring(commaPosition+1, schStep.length()); // Just pas the , and move to the next part
        }//Edn if
      }
      while(commaPosition >=0);//End Do

      // Save the schedule temp data to Rate, Temp & Soak
      schRate[iStep] = schTmp[0]; // Fixed location by extraction order
      schTemp[iStep] = schTmp[1];
      schSoak[iStep] = schTmp[2];
      iStep++; // Increment for next Step

      //Move to next section of the message
      message = message.substring(stepPosition+1, message.length());
    }
    else{// After the last ; is found, we have the schedule Name
      if(message.length() > 0)
        message.trim();
      message.toCharArray(schName,message.length()); //Copy to schedule name max buffer 
    }
    schLastStep = iStep - 1; // Set the Index for the last Step in the Firing Schedule
  }
  while(stepPosition >=0); 

  // Display Schedule for Review
  DisplaySchedule();

} //End Read Schedule Function

void DisplaySchedule()
{
  screen = 2; // Set Screen ID for Input 
  lcd.font( 1 ); // Use small font
  lcd.cls( BLACK, WHITE ); // clear screen

  lcd.print("Schedule Name: ");
  lcd.print(schName);
  lcd.print("\r");

  lcd.print("Schedule Steps: ");
  lcd.print(schLastStep + 1 );
  lcd.print("\n\r");

  for (int i=0; i <= schLastStep; i++)
  {
    lcd.print("Rate: ");
    lcd.print(schRate[i]);
    lcd.print(" Temp: ");
    lcd.print(schTemp[i]);
    lcd.print(" Soak: ");
    lcd.print(schSoak[i]);
    lcd.print("\r");  
  } // End For i Loop

  // Display Accept / Cancel Buttons
  lcd.picture( 276, 14, "Check.gif");
  lcd.picture( 276, 194, "Cancel.gif");
  lcd.touchZone( 20, 276, 14, 32, 32, 1);
  lcd.touchZone( 21, 276, 194, 32, 32, 1);

} //End Display Schedule Function

void processDisplaySchedule()
{
  if (( lcd.currentWidget == 20 ) && ( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if TouchZone 20 
    showRunScreen();
    displayStepInfo();
  }
  else if (( lcd.currentWidget == 21 ) && ( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if TouchZone 21 
    showPresetScreen(); // Go back to Load Preset Screen
  }
}

void processRunScreen()
{
  if (( lcd.currentWidget == 36 ) && ( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if TouchZone 20 
    // Shut it down!
    soakLED(0);
    tempLED(0);
    kilnLED(0);
    schRunning = 0;
    showPresetScreen();
  }
}

void showRunScreen()
{
  screen = 3; // Set Screen ID for Input 
  lcd.cls( BLACK, WHITE ); // clear screen
  iStep =0;

  // Fire Schedule Name
  lcd.font(0);
  lcd.xy(0,0);     
  lcd.print("Fire Schedule");

  // Analog Temp Meter
  lcd.string( 31, "Current Temp" ); // stringId 31
  lcd.theme( 11, 0,0,0,WHITE,0,0,0,0,0,0);
  lcd.analogMeter( 31, 120,0, 200, 150, 1, 0, 0, 2462, 11, 31, 0 );
  lcd.analogMeterColor(31,3,3,3,4,4,6);

  //Digital Meters
  lcd.fontw(7,"lcd24");
  lcd.theme(10, 1, 2, 3, 3, 3, 4, 4, 5, 111, 7); 

  // Temp Rate
  lcd.xy(0,63);     
  lcd.print("Rate");
  lcd.digitalMeter( 32, 50, 60, 60, 25, 14, 0000, 4, 0, 10);

  // Target Temp
  lcd.xy(0,98);     
  lcd.print("Temp");
  lcd.digitalMeter( 33, 50, 95, 60, 25, 14, 0000, 4, 0, 10);
  tempLED(0);

  // Soak
  lcd.xy(0,133);     
  lcd.print("Soak");
  lcd.digitalMeter( 34, 50, 130, 60, 25, 14, 000, 3, 0, 10);
  soakLED(0);

  // Step 
  lcd.xy(0,28);     
  lcd.print("Step");
  lcd.digitalMeter( 35, 50, 25, 60, 25, 14, 1, 2, 0, 10);

  //STOP
  lcd.theme(12, 1, 2, 3, 3, 3, 4, 4, 5, 0, 0);
  lcd.string( 32, "Stop" ); // stringId 32
  lcd.button( 36, 250, 170, 60, 60, 1, 0, 30, 12, 32);

  //Kiln Status
  kilnLED(0);

}//End showRunScreen()

void DisplayDisconnect()
{
  screen = 4; // Set Screen ID for Input 
  lcd.font( 1 ); // Use small font
  kilnLED(0); // Shut off the kiln
  probedisconnect = 0; //Reset probe disconnect
  lcd.cls( BLACK, WHITE ); // clear screen

  lcd.print("Probe Disconnected!");
  lcd.print("\r");

  lcd.print("Schedule terminated to protect Kiln.");
  lcd.print("\n\r");

  // Display Accept Button
  lcd.picture( 276, 14, "Check.gif");
  lcd.touchZone( 40, 276, 14, 32, 32, 1);


} //End Display Schedule Function

void processDisplayDisconnect()
{
  if (( lcd.currentWidget == 40 ) && ( lcd.currentInfo == PRESSED ) && ( lcd.currentData == 0 )) {// if TouchZone 20 
    showPresetScreen(); // Go back to Load Preset Screen
  }
}

void tempLED(boolean i)
{
  if (i == true){
    lcd.drawLed( 130, 107, 10, GREEN, WHITE);
  }
  else{
    lcd.drawLed( 130, 107, 10, BLACK, WHITE);
  }
}

void soakLED(boolean i)
{
  if (i == true){
    lcd.drawLed( 130, 142, 10, GREEN, WHITE);
  }
  else{
    lcd.drawLed( 130, 142, 10, BLACK, WHITE);
  }
}

void kilnLED(boolean i)
{
  if (i == true){
    lcd.drawLed( 300, 122, 12, RED, WHITE);
    digitalWrite(7, HIGH);
  }
  else{
    lcd.drawLed( 300, 122, 12, BLACK, WHITE);
    digitalWrite(7, LOW);
  }
}

int getTemp() //Get the current Temp
{
  int iTemp = (int) thermocouple.readFarenheit();
  if ( iTemp != 0){
    return iTemp;
  }
  else{
    return currentTemp;
  }
}

void displayStepInfo()
{
  lcd.wvalue(32, schRate[iStep]); //Set Rate Value
  lcd.wvalue(33, schTemp[iStep]); //Set Temp Value
  lcd.wvalue(34, schSoak[iStep]); //Set Soak Value
  lcd.wvalue(35, iStep+1); //Set Step Value

  // Set initals values for current step
  rampStart = minutes -1 ; //Set Start Time -1 to kick start ramp temp on first minute
  startTemp = (int) thermocouple.readFarenheit(); //Set Start Temp
  reachTemp = 0;
  soakLED(0);
  tempLED(0);
  schRunning = 1;
}




