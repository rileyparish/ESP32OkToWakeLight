#include <WiFi.h>
#include "time.h"

// NOTE: if local time fails to init from the NTP call, it's likely that NTP requests are being blocked by your router/ISP.

const char* ssid       = "YOUR_NETWORK";
const char* password   = "YOUR_NETWORK_PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long mstOffset_sec = -7 * 60 * 60;    // MST timezone
// const int daylightOffset_sec = 0;
const int  daylightOffset_sec = 3600;  // uncomment this when daylight savings begins. Probably.

const int BED_TIME_HOUR = 18;   // 19:00 24h time
const int BED_TIME_MINUTES = 55;
const int MORNING_TIME_HOUR = 7;
const int MORNING_TIME_MINUTE = 15;
const int NAPTIME_DURATION_MS = 1000 * 60 * 150;   // 150 minute nap (2.5 hrs)
unsigned long naptimeStart = 0;
bool isNaptime = false;

// These pins apparently can't be used at the same time as wifi: [4,0,2,15,13,12,14,27,25,26]
const int GREEN_PIN1 = 33;
const int GREEN_PIN2 = 19;
const int YELLOW_PIN1 = 32;
const int YELLOW_PIN2 = 5;
const int BUTTON_PIN = 21;

int notConnectedCounter = 0;

void greenLightsOn(){
  digitalWrite(YELLOW_PIN1, LOW);
  digitalWrite(YELLOW_PIN2, LOW);
  digitalWrite(GREEN_PIN1, HIGH);
  digitalWrite(GREEN_PIN2, HIGH);
}

void yellowLightsOn(){
  digitalWrite(GREEN_PIN1, LOW);
  digitalWrite(GREEN_PIN2, LOW);
  digitalWrite(YELLOW_PIN1, HIGH);
  digitalWrite(YELLOW_PIN2, HIGH);
}

void flashNoWifi(){
  digitalWrite(GREEN_PIN2, LOW);
  digitalWrite(YELLOW_PIN1, LOW);
  digitalWrite(GREEN_PIN1, HIGH);
  digitalWrite(YELLOW_PIN2, HIGH);
  delay(250);

  digitalWrite(GREEN_PIN1, LOW);
  digitalWrite(YELLOW_PIN2, LOW);
  digitalWrite(GREEN_PIN2, HIGH);
  digitalWrite(YELLOW_PIN1, HIGH);
  delay(250);
}

void flashNoLocalTime(){
  digitalWrite(GREEN_PIN1, LOW);
  digitalWrite(YELLOW_PIN1, LOW);
  digitalWrite(GREEN_PIN2, HIGH);
  digitalWrite(YELLOW_PIN2, HIGH);
  delay(150);

  digitalWrite(GREEN_PIN2, LOW);
  digitalWrite(YELLOW_PIN2, LOW);
  digitalWrite(GREEN_PIN1, HIGH);
  digitalWrite(YELLOW_PIN1, HIGH);
  delay(150);

  digitalWrite(GREEN_PIN1, LOW);
  digitalWrite(YELLOW_PIN1, LOW);
  digitalWrite(GREEN_PIN2, HIGH);
  digitalWrite(YELLOW_PIN2, HIGH);
  delay(150);

  digitalWrite(GREEN_PIN2, LOW);
  digitalWrite(YELLOW_PIN2, LOW);
  digitalWrite(GREEN_PIN1, HIGH);
  digitalWrite(YELLOW_PIN1, HIGH);
  delay(150);

  digitalWrite(GREEN_PIN2, LOW);
  digitalWrite(YELLOW_PIN2, LOW);
  digitalWrite(GREEN_PIN1, LOW);
  digitalWrite(YELLOW_PIN1, LOW);
}

bool isAfterBedtime(tm timeinfo){
  return timeinfo.tm_hour * 60 + timeinfo.tm_min >= BED_TIME_HOUR * 60 + BED_TIME_MINUTES;
}

bool isBeforeWakeTime(tm timeinfo){
  return timeinfo.tm_hour * 60 + timeinfo.tm_min <= MORNING_TIME_HOUR * 60 + MORNING_TIME_MINUTE;
}

void updateBedtimeIndicator()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    // we can't proceed if we don't have the time stored locally.
    // restart and try again.
    flashNoLocalTime();
    ESP.restart();
    return;
  }

  if(isAfterBedtime(timeinfo) || isBeforeWakeTime(timeinfo)){
    yellowLightsOn();
  }else{
    greenLightsOn();
  }

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
}

void setup(){
  Serial.begin(115200);
 
  //connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);

  pinMode(GREEN_PIN1, OUTPUT);
  pinMode(GREEN_PIN2, OUTPUT);
  pinMode(YELLOW_PIN1, OUTPUT);
  pinMode(YELLOW_PIN2, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  while(WiFi.status() != WL_CONNECTED) {
    // flash the lights while waiting for a wifi connection
    flashNoWifi();
    Serial.print(".");
    notConnectedCounter++;
    // attempt to connect 20 times and after that restart and try again
    if(notConnectedCounter > 20){
      ESP.restart();
    }
  }
  Serial.println(" CONNECTED");
 
  //init and get the time
  configTime(mstOffset_sec, daylightOffset_sec, ntpServer);
  updateBedtimeIndicator();

  //disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
}

void loop(){
  delay(100);
  // check if it's time to update the bedtime indicators yet
  if(!isNaptime){
    updateBedtimeIndicator();
  }

  // check if the naptime button is being pressed
  if(digitalRead(BUTTON_PIN) == LOW){
    naptimeStart = millis();
    yellowLightsOn();
    isNaptime = true;
    // debounce
    delay(200);
  }
  // check the current time to see if naptime is over
  if(isNaptime && millis() - naptimeStart > NAPTIME_DURATION_MS){
    greenLightsOn();
    isNaptime = false;
  }
}