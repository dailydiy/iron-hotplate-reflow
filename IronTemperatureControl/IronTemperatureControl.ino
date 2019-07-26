#include <Wire.h> 
#include <LiquidCrystal_I2C.h>

#define TOTALSTAGES 5
#define READY 0
#define PREHEAT 1
#define SOAKING 2
#define REFLOW 3
#define COOLING 4

#define MAX_ON_TIME 70
#define MAX_OFF_TIME 50

#define SYSTEMOFFSET 5

#define SSR 2
#define THERMOCOUPLE A0
#define CONTROLBUTTON 3
#define BUZZER 4

#define LCD_REFRESH_RATE 1000


int stageTemperatures[TOTALSTAGES] = { 30, 155, 185, 195, 75};// Target Temperature of Each stage in Degrees C
int stageDuration[TOTALSTAGES] = {1, 60, 60, 10, 1};//duration of each stage in seconds
String stageNames[TOTALSTAGES]={"Ready","Preheating","Soaking","Reflowing","Cooling"};
int currentStage = READY;
float systemTemperature,targetTemperature;
boolean temperatureReached;
unsigned long int achieveIn;
int startReflowSignal;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  // initialize //Serial communications at 9600 bps:
  //Serial.begin(9600);
  pinMode(SSR, OUTPUT);
  pinMode(BUZZER, OUTPUT);
  pinMode(CONTROLBUTTON, INPUT_PULLUP);
  digitalWrite(SSR, LOW);
  digitalWrite(BUZZER, LOW);
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Iron Reflow V1.1");  
  delay(5000);
  lcd.clear();
  displayStageName(currentStage);
}
float sysTemperature;

void loop() {


  sysTemperature=getMeanTemperature();
  
  
 
  if (currentStage == COOLING)
  {
    //shutdown the SSR
    digitalWrite(SSR, LOW);
    if(sysTemperature<stageTemperatures[currentStage])
    {
      currentStage = READY;
      lcd.clear();
      displayStageName(currentStage);
    
    }
    displayStageTemperature(currentStage,sysTemperature);
    
  }

  else if (currentStage == READY)
  {

    //shutdown the SSR    
   
    digitalWrite(SSR, LOW);
    //wait here for next signal
    //currentStage=currentStage%TOTALSTAGES;
    startReflowSignal = digitalRead(CONTROLBUTTON);//Check if user has pressed the button
    if (startReflowSignal == 0)
    {
      delay(500);
      startReflowSignal = digitalRead(CONTROLBUTTON);
      if (startReflowSignal == 0) // User Held the button for 300ms, equivalent to a long press
      {
        currentStage = PREHEAT;
        lcd.clear();
        displayStageName(currentStage);
        beepBuzzer()
      }
      

    }

    displayStageTemperature(currentStage,sysTemperature);
  }
  else
  {
    //targetTemperature = stageTemperatures[currentStage];
    ////Serial.print("Target Temperature is: ");
    ////Serial.println(targetTemperature);
    //achieveIn=stageDuration[currentStage];
    temperatureReached = achieveTemperature(currentStage);
    if (temperatureReached)
    {
      currentStage = currentStage + 1;
      if(currentStage==TOTALSTAGES)
      {
        currentStage=0;
      }
      lcd.clear();
      displayStageName(currentStage);
      beepBuzzer();
   
    }

  }


}

float getMeanTemperature()
{
  int i;
  int sum = 0;
  int sampleSize = 16;
  int shift = 4;
  for (i = 0; i < sampleSize; i++)
  {
    sum = sum + analogRead(THERMOCOUPLE);
    delay(2);//Wait for analogRead to Settle
  }
  return (sum >> shift) * (0.488);

}
unsigned long int stageStartTime=millis();
boolean achieveTemperature(int lstage)
{
  //lcd.clear();
  //displayStageName(lstage);
  systemTemperature = getMeanTemperature();
  float stageSlope,currentSlope;
  float stageStartTemperature=systemTemperature;//The tempearture we started the stage with.
  int ontime = MAX_ON_TIME;
  int offtime = MAX_OFF_TIME;
  unsigned long int timeElapsed;
  int targetTemperature = stageTemperatures[lstage];

  int lStageDuration=stageDuration[lstage];
  // do this until the target temperature is greater than the system temperature+offset
  stageStartTime=millis();// the time the stage started
  stageSlope=(targetTemperature-stageStartTemperature)/lStageDuration;// Required Slope of line for this stage in Degrees centrigrade/second
  // The system Offset allows some headroom while temperature is climbing, and reduces overshoot. 
  while (targetTemperature > systemTemperature + SYSTEMOFFSET) 
  {

    digitalWrite(SSR, HIGH); //Turn on the SSR
    delay(ontime);//Calculated below
    digitalWrite(SSR, LOW);
    delay(offtime);//Calculated below
    systemTemperature = getMeanTemperature();
    //Serial.println(systemTemperature);
    timeElapsed=millis()-stageStartTime;
    currentSlope=(systemTemperature-stageStartTemperature)* 1000 / timeElapsed; //Time elapsed is in milliseconds

    if(currentSlope>stageSlope)// slow down temperature gain
    {
      ontime=ontime-1;
      offtime=offtime+1;
    }
    if(currentSlope<stageSlope)// increase temperature gain
    {
      ontime=ontime+1;
      offtime=offtime-1;
    }

    if(ontime<0)
    {
      ontime=0;
    }

    if(offtime<0)
    {
      offtime=0;
    }

    if(ontime>MAX_ON_TIME)
    {
      ontime=MAX_ON_TIME;
    }

    if(offtime>MAX_OFF_TIME)
    {
      offtime=MAX_OFF_TIME;
    }

    displayStageTemperature(lstage,systemTemperature);


  }


  return true;// Temperature Achieved

}
void displayStageName(int lstage)
{
  
  lcd.setCursor(0,0);
  lcd.print(stageNames[lstage]);  
 

}
unsigned long int lastRefreshTime=millis();
void displayStageTemperature(int lstage, float lsystemTemperature)
{
  byte i;
  if(millis()-lastRefreshTime>LCD_REFRESH_RATE)
  {
    lcd.setCursor(0,1);
    for(i=0;i<7;i++)
     {
       lcd.print(" ");
     } 
    lcd.setCursor(0,1);
    lcd.print(int(lsystemTemperature));
    lcd.print("/");
    lcd.print(stageTemperatures[lstage]);
    lastRefreshTime=millis();
  }

}
void beepBuzzer()
{
  digitalWrite(BUZZER, HIGH);
  delay(50);
  digitalWrite(BUZZER, LOW);
}

