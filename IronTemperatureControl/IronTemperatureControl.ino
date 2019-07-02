#define TOTALSTAGES 5
#define READY 0
#define PREHEAT 1
#define SOAKING 2
#define REFLOW 3
#define COOLING 4

#define MAX_ON_TIME 10
#define MAX_OFF_TIME 200

#define SYSTEMOFFSET 10

#define SSR 13
#define THERMOCOUPLE A0
#define CONTROLBUTTON 3

int stageTemperatures[5] = {30, 150, 180, 205, 30};// Target Temperature of Each stage in Degrees C
int stageDuration[5] = {1, 60, 30, 45, 1};//duration of each stage in seconds

int currentStage = READY;
float systemTemperature,targetTemperature;
boolean temperatureReached;
unsigned long int achieveIn;
int startReflowSignal;

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(9600);
  pinMode(SSR, OUTPUT);
  digitalWrite(SSR, LOW);
}
float sysTemperature;

void loop() {

  
  sysTemperature=getMeanTemperature();
  Serial.println(sysTemperature);
  if (currentStage == COOLING)
  {
    //shutdown the SSR
    digitalWrite(SSR, LOW);
    currentStage = READY;
  }

  if (currentStage == READY)
  {
    //shutdown the SSR
    digitalWrite(SSR, LOW);
    //wait here for next signal
    //currentStage=currentStage%TOTALSTAGES;
    startReflowSignal = digitalRead(CONTROLBUTTON);//Check if user has pressed the button
    if (CONTROLBUTTON == 0)
    {
      delay(500);
      startReflowSignal = digitalRead(CONTROLBUTTON);
      if (CONTROLBUTTON == 0) // User Held the button for 300ms, equivalent to a long press
      {
        currentStage = PREHEAT;
      }

    }


  }
  else
  {
    targetTemperature = stageTemperatures[currentStage];
    achieveIn=stageDuration[currentStage];
    temperatureReached = achieveTemperature(targetTemperature,achieveIn);
    if (temperatureReached)
    {
      currentStage = currentStage + 1;
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
boolean achieveTemperature(float targetTemperature, int lStageDuration)
{
  
  systemTemperature = getMeanTemperature();
  float stageSlope,currentSlope;
  float stageStartTemperature=systemTemperature;//The tempearture we started the stage with.
  int ontime = MAX_ON_TIME;
  int offtime = MAX_OFF_TIME;
  unsigned long int timeElapsed;
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
    
    
  }

  
  return true;// Temperature Achieved

}
