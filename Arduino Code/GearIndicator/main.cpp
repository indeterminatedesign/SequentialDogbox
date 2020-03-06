#include <Arduino.h>
#include <EEPROM.h>

// make an array to save Sev Seg pin configuration of numbers
int ledArray[13][7] = {
    {1, 1, 1, 1, 1, 1, 0}, // 0 Neutral
    {0, 1, 1, 0, 0, 0, 0}, // 1
    {1, 1, 0, 1, 1, 0, 1}, // 2
    {1, 1, 1, 1, 0, 0, 1}, // 3
    {0, 1, 1, 0, 0, 1, 1}, // 4
    {1, 0, 1, 1, 0, 1, 1}, // 5
    {1, 0, 1, 1, 1, 1, 1}, // 6
    {1, 1, 1, 0, 0, 0, 0}, // 7
    {1, 1, 1, 1, 1, 1, 1}, // 8
    {1, 1, 1, 0, 0, 1, 1}, // 9
    {0, 0, 1, 0, 1, 0, 1}, // 10 Next
    {0, 0, 0, 0, 1, 0, 1}, //  11 Reverse
    {0, 0, 0, 1, 1, 1, 0}  // 12 Learn
};

//Array to save the values for the gear positions, pot value comma LED array index
int gearSettings[6][2] =
    {
        {500, 11}, //Reverse
        {418, 0}, //Neutral
        {336, 1}, //1st
        {256, 2}, //2nd
        {176, 3}, //3rd
        {100, 4}  //4th
};

//Array to save segment to digital io pin mapping, 2nd digit represents the segment letter numberical (a=0)
int segmentToPinLookup[8][2] =
    {
        {5, 0}, //A
        {7, 1}, //B
        {9, 2}, //C
        {6, 3}, //D
        {10, 4}, //E
        {12, 5},  //F
        {11, 6}, //G
        {8, 7}  //DP
};

//Variables to store time
unsigned long previousMillisButton = 0; // will store last time button was updated
int button = LOW;
unsigned long buttonInterval = 1000;
unsigned long previousMillisGearIndicator = 0; // will store last time LED was updated
unsigned long gearIndicatorInterval = 50;

//Initialize Other Variables
int shiftPotPin = 0;
int buttonPin = 2;
int filteredValue = 0;
int previousFilteredValue = 0;
int gearValueThreshold = 25;
int numberOfButtonPeriods = 0; //Stores the number of periods the button has been pressed to count time for programming
int currentGear = 0;

//function header
void DisplayGear(int);
int FilterValue(int, int);
void LearnGears();
void LoadGearSettings();
void StoreGearSettings();
void ResetDisplay (int);

//**********************************************************************
//**********************************************************************
void setup()
{

  Serial.begin(9600);

  // set pin modes
  pinMode(5, OUTPUT);  //A
  pinMode(7, OUTPUT);  //B
  pinMode(9, OUTPUT);  //C
  pinMode(6, OUTPUT);  //D
  pinMode(10, OUTPUT);  //E
  pinMode(12, OUTPUT); //F
  pinMode(11, OUTPUT); //G
  pinMode(8, OUTPUT); //DP
  pinMode(4, INPUT);

  //Reset the display, needed for common anode display
  ResetDisplay(HIGH);

  //Load the gear settings from Eprom
  LoadGearSettings();
}

//**********************************************************************
//**********************************************************************
void loop()
{
  unsigned long currentMillis = millis();

  //Check if Button is Pressed, if held for 5 counts then go into learning mode
  if ((currentMillis - previousMillisButton) >= buttonInterval)
  {
    previousMillisButton = currentMillis;
    int buttonState = digitalRead(buttonPin);
    Serial.println(buttonState);
    if (buttonState == HIGH)
    {
      Serial.write("Button Pushed");
      numberOfButtonPeriods++;
      if (numberOfButtonPeriods == 3)
      {
        numberOfButtonPeriods = 0;
        LearnGears();
      }
    }
  }

  if ((currentMillis - previousMillisGearIndicator) >= gearIndicatorInterval)
  {
    Serial.println("Check for gear change");
    previousMillisGearIndicator = currentMillis;

    int potVal = analogRead(shiftPotPin); // read the value from the sensor
    //read the sensor value using ADC
    filteredValue = FilterValue(potVal, filteredValue);
    Serial.println(filteredValue);

    for (int i = 0; i < 6; i++)
    {
      if ((filteredValue - gearValueThreshold <= gearSettings[i][0]) && filteredValue + gearValueThreshold >= gearSettings[i][0])
      {

        if (currentGear != gearSettings[i][1])
        {
        Serial.println("DisplayGear called");
        Serial.println(currentGear);
        Serial.println(gearSettings[i][1]);
        DisplayGear(gearSettings[i][1]);
        currentGear = gearSettings[i][1];
        }
      }
    }
  }
}

//**********************************************************************
// LearnGears
//**********************************************************************
void LearnGears()
{
  Serial.println("***Entered Learning Mode****");
  DisplayGear(12);        // Display L to indicate learning
  digitalWrite(segmentToPinLookup[7][0], LOW); //Turn on decimal point to indicate programming mode
  delay(5000);  //Wait 5 seconds before starting learning
  
  for (int i = 0; i < 6; i++)
  {
    DisplayGear(gearSettings[i][1]);
    digitalWrite(segmentToPinLookup[7][0], LOW); //Turn on decimal point to indicate programming mode
    int filteredValue = 0;
    int potVal = 0;
    int buttonState = LOW;
    Serial.println(buttonState);
    
    //Check for a button press and continuously read pot values thru the filter function
    while (buttonState == LOW)
    {
      buttonState = digitalRead(buttonPin);
      potVal = analogRead(shiftPotPin);
      filteredValue = FilterValue(potVal, filteredValue);
      delay(50);
      Serial.println("Learning Reading Pot Value");
      Serial.println(buttonState);
    }

    //store filtered value in array
    gearSettings[i][0] = filteredValue;

    //Display the next gear indicator
    DisplayGear(10);
    digitalWrite(segmentToPinLookup[7][0], LOW); //Turn on decimal point to indicate programming mode
    delay(1000);
  }

  int dp = HIGH;
  //Flash Decimal Point to show learning is complete
  for (int i = 0; i < 5; i++)
  {
    digitalWrite(segmentToPinLookup[7][0], dp); //Turn on decimal point to indicate programming mode
    dp = !dp;
    delay(150);
  }

  //Save the gear and pot values to eprom
  StoreGearSettings();
  Serial.println("Learning Complete");
}

//**********************************************************************
// Reset Display
// this functions sets all pins to high, for common anode 7 segments
//**********************************************************************
 void ResetDisplay (int state)
  {
    //Start at pin 4 and set the state of each pin
   for (int i = 5; i<13; i++)
   {
   digitalWrite(i,state);
   }
 }


//**********************************************************************
// Display Gear
// this functions writes values to the sev seg pins
//**********************************************************************
void DisplayGear(int number)
{
  ResetDisplay(HIGH);
  for (int j = 0; j < 7; j++)
  {
        //Common anode, switch logic if your 7 segement is commond cathode
    digitalWrite(segmentToPinLookup[j][0], abs(ledArray[number][j]-1));
    Serial.println("Display Gear Segment Calls");
    Serial.println(segmentToPinLookup[j][0]);
    Serial.println(!ledArray[number][j]);
  }
}

//**********************************************************************
//FilterValues 
// Applies filter to values being read from Pot
//**********************************************************************
int FilterValue(int newValue, int previousValue)
{
  float EMA_a = 0.6; // EMA Alpha
  return newValue = (EMA_a * previousValue) + ((1 - EMA_a) * newValue);
}

//**********************************************************************
//LoadGearSettings 
// Load gear settings from eeprom
//**********************************************************************
void LoadGearSettings()
{
  //Check to see if any settings have been stored to eeprom yet.  If no, then don't try to load from eeprom.
  int val = EEPROM.read(0);
if (val != 255)
{
  int eeAddress = 0;
//Read gear settings to eeprom
  for ( int i = 0; i < 6; i++)
  {
    Serial.print( "Read float from EEPROM: " );
    EEPROM.get(eeAddress, gearSettings[i][0]);
    eeAddress += __SIZEOF_INT__;
    Serial.println(gearSettings[i][0]);
  }
}
}

//**********************************************************************
//StoreGearSettings 
// Save gear settings to eeprom
//**********************************************************************
void StoreGearSettings()
{

int eeAddress = 0;
//Store each gear setting into eeprom
  for (int i = 0; i < 6; i++)
  {
    EEPROM.put(eeAddress, gearSettings[i][0]);
    eeAddress += __SIZEOF_INT__;
  }
}

