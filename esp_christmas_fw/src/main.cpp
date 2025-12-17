#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <config.h>

#define LED_PIN 3
#define NUM_LEDS 64

WiFiClient espClient;
PubSubClient client(espClient);

CRGB leds[NUM_LEDS];

void setup() {
  Serial.begin(115200);

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  client.setServer(BROKER_IP, BROKER_PORT);

  if (client.connect("ESP32Client", MQTT_USER, "")) {
    Serial.println("Connected to MQTT Broker!");
  } else {
    Serial.print("Failed to connect to MQTT Broker, rc=");
    Serial.print(client.state());
  }

  client.subscribe("data/sequence");

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
}

void loop() {
  client.loop();

  for(int hue = 0; hue < 255; hue += 1) {
    for(int i = 0; i < NUM_LEDS; i++) {
      leds[i] = CHSV(hue, 128, 255);
    }
    FastLED.show();
    delay(10);
  }
}
