#include <Adafruit_NeoPixel.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <time.h>
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
const color LED_COLOR_OFF{0, 0, 0};
const char* ntpServer = "pool.ntp.org";
const long  gmtOffsetSec = 3600;
const int   daylightOffsetSec = 3600;

// ========================
// GLOBALES
// ========================
int pinStateCurrent = LOW;
int pinStatePrevious = LOW;
int LIGHT_INTENSITY_TO_ACTIVATE_LED = 10;
int DELAY_BETWEEN_PIXEL_ACTIVATION = 5;
int LED_ON_DURATION = 30000; // ms
int MOTION_VALIDATION_DELAY_MS = 800; // ms
bool LED_ENABLED = true;
color LED_COLOR_ON{184, 12, 207};
Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);
unsigned long lastStatusMillis = 0;
const int STATUS_INTERVAL = 1800000;

// ========================
// WiFi & MQTT
// ========================
// definition in secrets.h
WiFiClient espClient;
PubSubClient client(espClient);

enum class MqttCommandType {
  SET_LED_COLOR,
  SET_LIGHT_INTENSITY,
  SET_DELAY_BETWEEN_PIXEL,
  SET_LED_ON_DURATION,
  SET_LED_ENABLED
};

struct MqttCommand {
  MqttCommandType type;
  union {
    int intValue;
    struct {
      uint8_t r;
      uint8_t g;
      uint8_t b;
    } color;
  };
};
QueueHandle_t mqttCommandQueue;

// ========================
// TASKS
// ========================
TaskHandle_t wifiMqttClientTask;

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
  pinMode(PIN_HCSR501, INPUT);
  
  ws2812b.begin();
  ws2812b.setBrightness(80);

  mqttCommandQueue = xQueueCreate(100, sizeof(MqttCommand));
  
  xTaskCreatePinnedToCore(
    wifiMqttClientTaskWorker,
    "WifiMqttClientConnectorTask",
    10000,
    NULL,
    0,
    &wifiMqttClientTask,
    0
  );
}

// ========================
// LOOP
// ========================
void loop() {
  ws2812b.clear();
  ws2812b.show();

  processMqttCommands();
  
  if(LED_ENABLED && isMotionDetected() && checkLightIntensity()){
    led(0, NUM_PIXELS, LED_COLOR_ON);

    unsigned long startTime = millis();
    while(millis() - startTime < LED_ON_DURATION) {
      if(isMotionDetected())
        startTime = millis();
    }
    
    led(NUM_PIXELS, 0, LED_COLOR_OFF);
  }
}

// ========================
// WIFI MQTTCLIENT TASK
// ========================
void wifiMqttClientTaskWorker( void *parameter )
{
  struct tm timeinfo;
  configTime(gmtOffsetSec, daylightOffsetSec, ntpServer);
  
  client.setServer(mqtt_broker, mqtt_port);
  client.setCallback(mqttCallback);
  
  for(;;) {
    if( WiFi.status() != WL_CONNECTED)
    {
      WiFi.begin(ssid, password);

      while(WiFi.status() != WL_CONNECTED) {
        delay(250);
      }
    }

    if( !client.connected() )
    {
      String clientId = "esp32-ledStairs-client-" + String(WiFi.macAddress());
      client.connect(clientId.c_str());

      if(client.connected()){
        for(int i = 0; i < NUM_SUB_TOPICS; i++){
          client.subscribe(SUB_TOPICS[i]);
        }

        // GET RETAINED MESSAGES
        unsigned long start = millis();
        while (millis() - start < 500) {
          client.loop();
          vTaskDelay(10 / portTICK_PERIOD_MS);
        }
      }
    }
    else
    {
      client.loop();
      publishStatus();
    }
  }
  vTaskDelete(NULL);
}

void mqttCallback(char *topic, byte *payload, unsigned int length)
{
  char msg[length + 1];
  memcpy(msg, payload, length);
  msg[length] = '\0';

  MqttCommand cmd;

  // ===== LED COLOR =====
  if (strcmp(topic, SUB_TOPIC_LED_COLOR) == 0) {
    int r, g, b;
    if (sscanf(msg, "[%d,%d,%d]", &r, &g, &b) == 3) {
      cmd.type = MqttCommandType::SET_LED_COLOR;
      cmd.color.r = constrain(r, 0, 255);
      cmd.color.g = constrain(g, 0, 255);
      cmd.color.b = constrain(b, 0, 255);
    } else {
      return;
    }
  }
  // ===== INT TOPICS =====
  else if (isValidInt(msg, length)) {
    int val = atoi(msg);
    cmd.intValue = val;

    if (strcmp(topic, SUB_TOPIC_LIGHT_INTENSITY) == 0)
      cmd.type = MqttCommandType::SET_LIGHT_INTENSITY;
    else if (strcmp(topic, SUB_TOPIC_DELAY_BETWEEN_PIX) == 0)
      cmd.type = MqttCommandType::SET_DELAY_BETWEEN_PIXEL;
    else if (strcmp(topic, SUB_TOPIC_LED_ON_DURATION) == 0)
      cmd.type = MqttCommandType::SET_LED_ON_DURATION;
    else if (strcmp(topic, SUB_TOPIC_LED_ENABLED) == 0)
      cmd.type = MqttCommandType::SET_LED_ENABLED;
    else
      return;
  }
  // ===== STATE TOPIC =====
  else if (strcmp(topic, SUB_TOPIC_LED_STATE_REQUEST) == 0) {
    if(!client.connected()) return;

    StaticJsonDocument<64> doc;
    char buffer[64];
    
    {
      StaticJsonDocument<64> doc;
      doc["lightIntensity"] = LIGHT_INTENSITY_TO_ACTIVATE_LED;
      size_t n = serializeJson(doc, buffer);
      client.publish(PUB_TOPIC_LIGHT_INTENSITY, buffer, n);
    }

    {
      StaticJsonDocument<64> doc;
      doc["delayBetweenPixel"] = DELAY_BETWEEN_PIXEL_ACTIVATION;
      size_t n = serializeJson(doc, buffer);
      client.publish(PUB_TOPIC_DELAY_BETWEEN_PIX, buffer, n);
    }

    {
      StaticJsonDocument<64> doc;
      doc["ledOnDuration"] = LED_ON_DURATION;
      size_t n = serializeJson(doc, buffer);
      client.publish(PUB_TOPIC_LED_ON_DURATION, buffer, n);
    }

    {
      StaticJsonDocument<64> doc;
      doc["ledEnabled"] = LED_ENABLED;
      size_t n = serializeJson(doc, buffer);
      client.publish(PUB_TOPIC_LED_ENABLED, buffer, n);
    }

    {
      StaticJsonDocument<64> doc;
      JsonArray color = doc.createNestedArray("ledColor");
      color.add(LED_COLOR_ON.r);
      color.add(LED_COLOR_ON.g);
      color.add(LED_COLOR_ON.b);
      size_t n = serializeJson(doc, buffer);
      client.publish(PUB_TOPIC_LED_COLOR, buffer, n);
    }
  }

  xQueueSend(mqttCommandQueue, &cmd, 0);
}


bool isValidInt(const char *str, int length){
  for(int i = 0; i < length; i++){
    if(!isDigit(str[i])) return false;
  }

  return true;
}

void publishStatus() {
  unsigned long nowMillis = millis();
  if (nowMillis - lastStatusMillis >= STATUS_INTERVAL || lastStatusMillis == 0) {
    lastStatusMillis = nowMillis;

    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) return;
    
    time_t currentTime = mktime(&timeinfo);
    time_t nextUpdateTime = currentTime + ( STATUS_INTERVAL / 1000 );

    struct tm nextTimeinfo;
    localtime_r(&nextUpdateTime, &nextTimeinfo);

    char dateTime[32];
    snprintf(dateTime, sizeof(dateTime), "%04d-%02d-%02d %02d:%02d:%02d",
      nextTimeinfo.tm_year + 1900,
      nextTimeinfo.tm_mon + 1,
      nextTimeinfo.tm_mday,
      nextTimeinfo.tm_hour,
      nextTimeinfo.tm_min,
      nextTimeinfo.tm_sec
    );
    
    StaticJsonDocument<128> doc;
    doc["online"] = true;
    doc["nextUpdate"] = dateTime;

    char buffer[128];
    size_t n = serializeJson(doc, buffer);

    if (!client.connected()) return;
    client.publish(PUB_TOPIC_LED_STATUS, (const uint8_t*)buffer, n, true);
  }
}


// ========================
// NORMAL TASK
// ========================
bool isMotionDetected() {
  static unsigned long motionStart = 0;
  static bool waitingForValidation = false;

  pinStatePrevious = pinStateCurrent;
  pinStateCurrent = digitalRead(PIN_HCSR501);

  if (pinStatePrevious == LOW && pinStateCurrent == HIGH) {
    motionStart = millis();
    waitingForValidation = true;
  }

  if (waitingForValidation && (millis() - motionStart >= MOTION_VALIDATION_DELAY_MS)) {
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

void processMqttCommands()
{
  MqttCommand cmd;
  if (xQueueReceive(mqttCommandQueue, &cmd, 0))
  {
    switch (cmd.type) {

      case MqttCommandType::SET_LED_COLOR:
        LED_COLOR_ON = {cmd.color.r, cmd.color.g, cmd.color.b};
        break;

      case MqttCommandType::SET_LIGHT_INTENSITY:
        LIGHT_INTENSITY_TO_ACTIVATE_LED = cmd.intValue;
        break;

      case MqttCommandType::SET_DELAY_BETWEEN_PIXEL:
        DELAY_BETWEEN_PIXEL_ACTIVATION = cmd.intValue;
        break;

      case MqttCommandType::SET_LED_ON_DURATION:
        LED_ON_DURATION = cmd.intValue;
        break;

      case MqttCommandType::SET_LED_ENABLED:
        cmd.intValue == 0 ? LED_ENABLED = false : LED_ENABLED = true;
        break;
    }
  }
}
