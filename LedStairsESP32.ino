#include <Adafruit_NeoPixel.h>

#define PIN_WS2812B 32
#define NUM_PIXELS 300

#define PIN_HCSR501 33
int pinStateCurrent = LOW;
int pinStatePrevious = LOW;

#define PIN_PHOTORESISTOR 25
int lightIntensityToActivateLED = 50;

Adafruit_NeoPixel ws2812b(NUM_PIXELS, PIN_WS2812B, NEO_GRB + NEO_KHZ800);

void setup() {
  Serial.begin(115200);
  ws2812b.begin();
  ws2812b.setBrightness(80);

  pinMode(PIN_HCSR501, INPUT);
}

void loop() {
  ws2812b.clear();
  ws2812b.show();

  pinStatePrevious = pinStateCurrent;
  pinStateCurrent = digitalRead(PIN_HCSR501);

  int photoValue = analogRead(PIN_PHOTORESISTOR);
  int lightIntensityPercentage = map(photoValue, 4095, 0, 0, 100);

  if(pinStatePrevious == LOW && pinStateCurrent == HIGH) {
    Serial.println("Motion detected");
    Serial.print("Light intensity: ");
    Serial.println(lightIntensityPercentage);
    if (lightIntensityPercentage < lightIntensityToActivateLED) {
      for(int pixel = 0; pixel < NUM_PIXELS; pixel++){
        ws2812b.setPixelColor(pixel, ws2812b.Color(184, 12, 207));
        ws2812b.show();

        delay(5);
      }
      delay(30000);
    }
  }
}
