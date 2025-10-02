# ESP32 LED Motion Light with MQTT

This project controls a **WS2812B LED strip** using an **ESP32**, an **HC-SR501 PIR motion sensor**, and a **photoresistor (LDR)**.  
The LEDs only turn on when **motion is detected** **and** the **ambient light level is low**.  

Additionally, an **MQTT interface** is integrated, allowing real-time configuration of various parameters.

---

## Features
- **Light level check**: Uses the photoresistor to detect if it’s dark enough.  
- **Motion detection**: The HC-SR501 detects nearby motion.
- **LED strip**: Lights up when motion is detected and if it’s dark enough.
- **LED control**: WS2812B LED strip control with customizable settings.  
- **MQTT integration**: Configure parameters via MQTT topics.  

### MQTT Topics
- "LIGHT_INTENSITY" – Threshold value for brightness detection (photoresistor).  
- "DELAY_BETWEEN_PIX" – Delay between lighting up each individual LED.  
- "LED_ON_DURATION" – Duration the LEDs stay on after motion is detected.  
- "LED_COLOR" – LED color configuration (e.g., RGB values).  
- "LED_ENABLED" – Enable/disable LED function (`1` = on, `0` = off).  

---

## Components
- ESP32  
- WS2812B LED strip  
- HC-SR501 motion sensor  
- Photoresistor (LDR)  
- 5V power supply  
- Jumper wires  
- **1000 µF capacitor** (protects LED strip from voltage spikes)  
- **10 kΩ resistor** (for LDR voltage divider)  
- **470 Ω resistor** (in series with LED data line)

---

## Use Cases
- Automatic **lighting when motion is detected in the dark** (e.g., hallway, bedroom, basement).  
- MQTT-controlled **ambient or mood lighting**.  

---

## License
This project is licensed under the **MIT License**.  
