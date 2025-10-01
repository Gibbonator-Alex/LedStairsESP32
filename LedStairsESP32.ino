#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include "secrets.h"

// ========================
// GPIO
// ========================
#define PIN_WS2812B 33
#define PIN_HCSR501 25
#define PIN_PHOTORESISTOR 32

// ========================
// STRUCT & CONSTANTS
// ========================
struct color{
  int r;
  int g;
  int b;
};

const int NUM_PIXELS = 182;
const int CONNECT_TIMEOUT = 5000;
const color LED_COLOR_OFF{0, 0, 0};

// ========================
// GLOBALES
// ========================
int pinStateCurrent = LOW;
int pinStatePrevious = LOW;
int LIGHT_INTENSITY_TO_ACTIVATE_LED = 20;
int DELAY_BETWEEN_PIXEL_ACTIVATION = 5;
int LED_ON_DURATION = 30000; // seconds
bool LED_ENABLED = true;

color LED_COLOR_ON{184, 12, 207};

Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

WiFiClient espClient;
PubSubClient client(espClient);

// ========================
// WiFi & MQTT
// ========================
// definition in secrets.h

// ========================
// PROTOTYPES
// ========================
void led(const int& fromPixel, const int& toPixel, const color c);
bool isMotionDetected();
bool checkLightIntensity();
void mqttCallback(char *topic, byte *payload, unsigned int length);
bool isValidInt(const char *str, int length);

// ========================
// SETUP
// ========================
void setup() {
  ws2812b.begin();
  ws2812b.setBrightness(80);

  ensureWiFiConnected();

  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqttCallback);
  ensureMqttConnected();

  pinMode(PIN_HCSR501, INPUT);
}

// ========================
// LOOP
// ========================
void loop() {
  ensureWiFiConnected();
  ensureMqttConnected();
  
  ws2812b.clear();
  ws2812b.show();

  client.loop();
  
  if(LED_ENABLED && isMotionDetected() && checkLightIntensity()){
    led(0, NUM_PIXELS, LED_COLOR_ON);

    // Keep LEDs on for LED_ON_DURATION, allowing MQTT callbacks to run
    unsigned long startTime = millis();
    while(millis() - startTime < LED_ON_DURATION) {
      client.loop();
    }
    
    led(NUM_PIXELS, 0, LED_COLOR_OFF);
  }
}

// ========================
// FUNCTIONS
// ========================
void ensureWiFiConnected(){
  if(WiFi.status() != WL_CONNECTED){
    WiFi.begin(ssid, password);
    unsigned long startAttemptTime = millis();
    while(WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < CONNECT_TIMEOUT){
      ws2812b.fill(ws2812b.Color(255, 0, 0), 0, 10);
      ws2812b.show();
      delay(250);
      ws2812b.clear();
      ws2812b.show();
      delay(250);
    }
  }
}

void ensureMqttConnected(){
  if(!client.connected()){
    String client_id = "esp32-client-" + String(WiFi.macAddress());
    
    unsigned long startAttemptTime = millis();
    while(!client.connect(client_id.c_str()) && millis() - startAttemptTime < CONNECT_TIMEOUT){
      ws2812b.fill(ws2812b.Color(255, 0, 0), 0, 10);
      ws2812b.show();
      delay(250);
      ws2812b.clear();
      ws2812b.show();
      delay(250); 
    }

    if(client.connected()){
      for(int i = 0; i < NUM_TOPICS; i++){
        client.subscribe(TOPICS[i]);
      }
    }
  }
}

bool isMotionDetected() {
  static unsigned long motionStart = 0;
  static bool waitingForValidation = false;

  pinStatePrevious = pinStateCurrent;
  pinStateCurrent = digitalRead(PIN_HCSR501);

  if (pinStatePrevious == LOW && pinStateCurrent == HIGH) {
    motionStart = millis();
    waitingForValidation = true;
  }

  if (waitingForValidation && (millis() - motionStart >= 1500)) {
    waitingForValidation = false;
    if (digitalRead(PIN_HCSR501) == HIGH) {
      return true;
    }
  }

  return false;
}

bool checkLightIntensity(){
  int photoValue = analogRead(PIN_PHOTORESISTOR);
  int lightIntensityPercentage = map(photoValue, 4095, 0, 0, 100);
  return (lightIntensityPercentage < LIGHT_INTENSITY_TO_ACTIVATE_LED);
}

void led(const int& fromPixel, const int& toPixel, const color c){
  int steps = abs(toPixel - fromPixel);
  int dir   = (toPixel > fromPixel) ? 1 : -1;

  for (int i = 0; i <= steps; i++) {
    int pixel = fromPixel + i * dir;
    ws2812b.setPixelColor(pixel, ws2812b.Color(c.r, c.g, c.b));
    ws2812b.show();
    
    delay(DELAY_BETWEEN_PIXEL_ACTIVATION);
  }
}

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';
  
  // ======= LED Color Topic =======
  if(strcmp(topic, TOPIC_LED_COLOR) == 0) {
    int r, g, b;
    if(sscanf(msg, "[%d,%d,%d]", &r, &g, &b) == 3){
      r = constrain(r, 0, 255);
      g = constrain(g, 0, 255);
      b = constrain(b, 0, 255);
      LED_COLOR_ON = {r, g, b};
    }
    else {
      return;
    }
  }
  // ======= Number-Topics =======
  else {
    if(!isValidInt(msg, length)) return;
    int val = atoi(msg);
    
    if(strcmp(topic, TOPIC_LIGHT_INTENSITY) == 0) {
      LIGHT_INTENSITY_TO_ACTIVATE_LED = val;
    } 
    else if(strcmp(topic, TOPIC_DELAY_BETWEEN_PIX) == 0) {
      DELAY_BETWEEN_PIXEL_ACTIVATION = val;
    } 
    else if(strcmp(topic, TOPIC_LED_ON_DURATION) == 0) {
      LED_ON_DURATION = val;
    }
    else if(strcmp(topic, TOPIC_LED_ENABLED) == 0) {
      if(val == 1){
        LED_ENABLED = true;
      } else if(val == 0){
        LED_ENABLED = false;
      }
    }
  }
}

bool isValidInt(const char *str, int length){
  for(int i = 0; i < length; i++){
    if(!isDigit(str[i])) return false;
  }

  return true;
}
