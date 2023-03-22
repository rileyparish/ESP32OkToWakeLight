#include <WiFi.h>
#include "time.h"

// NOTE: if local time fails to init from the NTP call, it's likely that NTP requests are being blocked by your router/ISP.

const char* ssid       = "YOUR_NETWORK";
const char* password   = "YOUR_NETWORK_PASSWORD";

const char* ntpServer = "pool.ntp.org";
const long  mstOffset_sec = -7 * 60 * 60;       // MST timezone
// const int   daylightOffset_sec = 0;
const int   daylightOffset_sec = 3600;   // uncomment this when daylight savings begins. Probably.

const int BED_TIME_HOUR = 18;     // 24h time
const int BED_TIME_MINUTES = 49;
const int MORNING_TIME_HOUR = 7;
const int MORNING_TIME_MINUTE = 19;
const int NAPTIME_DURATION_MS = 1000 * 60 * 60;     // 1h minute nap
unsigned long naptimeStart = 0;
bool isNaptime = false;

// These pins apparently can't be used at the same time as wifi: [4,0,2,15,13,12,14,27,25,26]
const int GREEN_PIN1 = 33;
const int GREEN_PIN2 = 18;
const int YELLOW_PIN1 = 32;
const int YELLOW_PIN2 = 5;
const int BUTTON_PIN = 21;

const int GREEN_BRIGHTNESS = 70;
const int YELLOW_BRIGHTNESS = 70;

int notConnectedCounter = 0;

const int freq = 5000;
const int greenLedChannel = 1;
const int yellowLedChannel = 2;
const int resolution = 8;

void greenLightsOn(){
    ledcWrite(yellowLedChannel, LOW);
    ledcWrite(greenLedChannel, GREEN_BRIGHTNESS);
}

void yellowLightsOn(){
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, YELLOW_BRIGHTNESS);
}

void flashNoWifi(){
    // both off
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, LOW);
    // green on
    ledcWrite(greenLedChannel, GREEN_BRIGHTNESS);
    delay(250);
    // green off, yellow on
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, YELLOW_BRIGHTNESS);
    delay(250);
    // yellow off, green on
    ledcWrite(yellowLedChannel, LOW);
    ledcWrite(greenLedChannel, GREEN_BRIGHTNESS);
    delay(250);
    // green off, yellow on
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, YELLOW_BRIGHTNESS);
    delay(250);
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, LOW);
}

void flashNoLocalTime(){
    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, LOW);

    ledcWrite(greenLedChannel, GREEN_BRIGHTNESS);
    ledcWrite(yellowLedChannel, YELLOW_BRIGHTNESS);
    delay(150);

    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, LOW);
    delay(150);

    ledcWrite(greenLedChannel, GREEN_BRIGHTNESS);
    ledcWrite(yellowLedChannel, YELLOW_BRIGHTNESS);
    delay(150);

    ledcWrite(greenLedChannel, LOW);
    ledcWrite(yellowLedChannel, LOW);
    delay(150);
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

    ledcSetup(greenLedChannel, freq, resolution);
    ledcSetup(yellowLedChannel, freq, resolution);
    
    //connect to WiFi
    Serial.printf("Connecting to %s ", ssid);
    WiFi.begin(ssid, password);

    ledcAttachPin(GREEN_PIN1, greenLedChannel);
    ledcAttachPin(GREEN_PIN2, greenLedChannel);
    ledcAttachPin(YELLOW_PIN1, yellowLedChannel);
    ledcAttachPin(YELLOW_PIN2, yellowLedChannel);
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