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
const char *TOPIC_LIGHT_INTENSITY   = "";
const char *TOPIC_DELAY_BETWEEN_PIX = "";
const char *TOPIC_LED_ON_DURATION   = "";
const char *TOPIC_LED_COLOR         = "";
const char *TOPIC_LED_ENABLED       = "";

const char* TOPICS[] = {
  TOPIC_LIGHT_INTENSITY,
  TOPIC_DELAY_BETWEEN_PIX,
  TOPIC_LED_ON_DURATION,
  TOPIC_LED_COLOR,
  TOPIC_LED_ENABLED
};
const int NUM_TOPICS = sizeof(TOPICS) / sizeof(TOPICS[0]);
#endif
