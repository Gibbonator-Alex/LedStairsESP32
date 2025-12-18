#ifndef SECRETS_H
#define SECRETS_H

// ========================
// WIFI
// ========================
const char *ssid = "";
const char *password = "";

// ========================
// MQTT BROKER
// ========================
const char *mqtt_broker   = "";
const char *mqtt_username = "";
const char *mqtt_password = "";
const int   mqtt_port     = 1883;
const char *SUB_TOPIC_LIGHT_INTENSITY   = "";
const char *SUB_TOPIC_DELAY_BETWEEN_PIX = "";
const char *SUB_TOPIC_LED_ON_DURATION   = "";
const char *SUB_TOPIC_LED_COLOR         = "";
const char *SUB_TOPIC_LED_ENABLED       = "";
const char *SUB_TOPIC_LED_STATE_REQUEST = "";

const char *PUB_TOPIC_LIGHT_INTENSITY   = "";
const char *PUB_TOPIC_DELAY_BETWEEN_PIX = "";
const char *PUB_TOPIC_LED_ON_DURATION   = "";
const char *PUB_TOPIC_LED_COLOR         = "";
const char *PUB_TOPIC_LED_ENABLED       = "";

const char* SUB_TOPICS[] = {
  SUB_TOPIC_LIGHT_INTENSITY,
  SUB_TOPIC_DELAY_BETWEEN_PIX,
  SUB_TOPIC_LED_ON_DURATION,
  SUB_TOPIC_LED_COLOR,
  SUB_TOPIC_LED_ENABLED,
  SUB_TOPIC_LED_STATE_REQUEST
};
const int NUM_SUB_TOPICS = sizeof(SUB_TOPICS) / sizeof(SUB_TOPICS[0]);
#endif
