#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <EEPROM.h>
#include "HX711.h"

//SCALE
HX711 scale;
#define WEIGHT_BUFFER_SIZE 15
#define MAXIMUM_WEIGHT_RANGE 0.3    // [kg]
#define MINIMUM_WEIGHT 45           // [kg]
float weight_buffer[WEIGHT_BUFFER_SIZE];

//SOFTWARE timing
unsigned long finishedSetup = 0;
unsigned long lastInterrupt = 0;
unsigned long lastblink = 0;

//FLAGS and VARIOUS
int current_shift_led = 0;
long last_sample_time = 0;
const int user_count = 4;
int currentUser = -1;
bool flag = false, updating = false;
int phase = 0;
String timens;
float weight;
String authKey, authKeyMessages;

//CONNECTIVITY
WiFiUDP Udp;
const char* ssid     = "YOURSSID";
const char* password = "YOURPASSWORD";

String host_google = "www.googleapis.com";
unsigned int localPort = 8888;
const int httpPort = 443;

//HARDWARE
#define LED0 14 //D5
#define LED1 12 //D6
#define LED2 13 //D7
#define LED3 D3 //D3
#define INTERRUPT_PIN D8
#define DISPLAY_SDA_PIN D5
#define DISPLAY_SCL_PIN D6

//NTP
#define TIME_ZONE 1   //for my city; change according to your zone
static const char ntpServerName[] = "us.pool.ntp.org";

void setup() {
  Serial.begin(115200);
  EEPROM.begin(512);
  delay(10);

  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  pinMode(LED2, OUTPUT);
  pinMode(LED3, OUTPUT);

  allLedOn();
  
  WiFi.hostname("Smart Scale");

  IPAddress ip(192, 168, 1, 10);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(ip, gateway, subnet, gateway);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("\nWiFi connected");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("");

  Udp.begin(localPort);
  delay(20);

  pinMode(INTERRUPT_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(INTERRUPT_PIN), handleInterrupt, RISING);

  time_t mtime = getNtpTime();
  int messTokenStatus = refreshTokenMessages();

  if (mtime == 0 || messTokenStatus < 0) {
    send_push("dev", "NTP error", "NTP didn't respond.");
    ESP.deepSleep(0);
  }

  timens = String(mtime) + "000000000"; //Google Fit API requires nanoseconds...

  delay(50);

  scale.begin(5, 4);
  scale.set_scale(-20155.7);  //tare factor; obtain this via the calibrating sketch
  scale.tare();

  yield();
  OTA_setup();

  allLedOff();
  finishedSetup = millis();
}

void loop() {
  ArduinoOTA.handle();
  if (updating) {
    blink_update();
    if (flag) {
      updating = false;
      flag = false;
      allLedOff();
      yield();
    }
  }
  else {
    switch (phase) {
      case 0: //user selection
        blink_led();
        checkPhaseChange();
        
        if (flag) {
          currentUser++;
          if (currentUser == user_count) updating = true;
          currentUser = currentUser % user_count;
          current_shift_led = currentUser;
          allLedOff();
          flag = false;
        }
        break;
      case 1:                     //weighting phase
        if (flag) sleep();        //sleep if button has been pressed
        shift_led();              //move led to the next one
        checkWeight();            //read weight
        break;
    }
    checkSleep();                 //check if enought time has elapsed, if so sleep
  }
  delay(10);
}

void checkSleep() {
  if (millis() > 35000) {
    String body = String("Peso: ") + weight + " kg;  range: " + get_range();
    send_push("Errore - inaffidabile", body);
    sleep();                      //notify user before sleeping
  }
}

void handleInterrupt() {
  if (millis() - lastInterrupt > 300) {
    flag = true;
    lastInterrupt = millis();
  }
}

void checkPhaseChange() {
  if ((get_weight() > 20) && !updating) {
    phase = 1;                  //switch from "user selection" to "weighting" phase if weight>20 & it's updating the software
    allLedOff();
    flag = false;
  }
  if (((millis() - finishedSetup) > 20000) && !updating) sleep();
}

void checkWeight() {
  weight = get_weight();
  add_to_buffer(weight);

  if (weight > MINIMUM_WEIGHT && get_range() < MAXIMUM_WEIGHT_RANGE) {      //weight is accepted if greater than threshold and if 15 readings give a consistent mean
    allLedOn();
    float post_weight = get_mean();
    
    if (currentUser > -1) {
      int postStatus = postWeight(String(post_weight, 2));
      
      if (postStatus > 0) {
        String body = String("New weight registered: ") + String(post_weight, 2) + " kg.\n" + getPhrase(post_weight);
        send_push(String("Scale - ") + getTopic(), body);
      }
      else send_push("Google Fit Error", "Weight of " + String(post_weight, 2) + " kg acquired but not uploaded. Error n.: " + postStatus);
      sleep();

    } else {
      send_push("generic", "Scale - Generic", "Weight: " + String(post_weight, 2) + " kg.\n" + getJoke());
      sleep();
    }
  }
}

void sleep() {
  Serial.println("\nGoing to sleep");

  scale.power_down();
  delay(50);

  for (int i = 0; i < 7; i++) {
    allLedOn();
    delay(150);
    allLedOff();
    delay(150);
  }

  delay(1000);
  ESP.deepSleep(0);
}

void OTA_setup() {
  ArduinoOTA.setHostname("Scale");
  ArduinoOTA.setPassword("otapassword");

  ArduinoOTA.onStart([]() {
    allLedOff();
    send_push("dev", "Update", "Begin uploading");
    Serial.println("Start");
  });

  ArduinoOTA.onEnd([]() {
    updating = false;
    send_push("dev", "Update", "Fnished uploading");
    Serial.println("\nEnd");
  });
  
  ArduinoOTA.begin();
}
