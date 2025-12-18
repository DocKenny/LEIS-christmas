#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <config.h>
#include <helper.h>

#define MAX_IMAGES 256

#define LED_PIN 3
#define NUM_LEDS 64
#define BUTTON_PIN 0 // TODO: Choose the correct pin

WiFiClient espClient;
PubSubClient client(espClient);

CRGB leds[NUM_LEDS];

uint8_t pixelData[MAX_IMAGES][NUM_LEDS * 3];

DeviceState state = IDLE;

uint16_t imageCount = 0;
uint16_t currentImage = 0;
uint16_t fps = 20;
unsigned long lastFrameTime = 0;

unsigned long stateTimestamp = 0;
bool responseReceived = false;

bool buttonPressed();

void sendStateRequest();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void connectWiFiAndMQTT();

void disconnectAll();

void updateLED();

void setup() {
  Serial.begin(115200);

  // LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.setBrightness(255);
  FastLED.clear(true);

  // Button
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // WiFi completely off at boot
  WiFi.mode(WIFI_OFF);

  // MQTT setup (no connection yet)
  client.setServer(BROKER_IP, BROKER_PORT);
  client.setCallback(mqttCallback);

  Serial.println("Setup complete, waiting for button press");
}

// void loop() {
//   client.loop();

//   for(int hue = 0; hue < 255; hue += 1) {
//     for(int i = 0; i < NUM_LEDS; i++) {
//       leds[i] = CHSV(hue, 128, 255);
//     }
//     FastLED.show();
//     delay(10);
//   }
// }

void loop() {
  switch (state) {

    case IDLE:
      if (buttonPressed()) {
        connectWiFiAndMQTT();
        state = CONNECTING;
      }

      updateLED();
      break;

    case CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        Serial.println("WiFi connected");

        client.setServer(BROKER_IP, BROKER_PORT);
        client.setCallback(mqttCallback);

        if (client.connect("ESP32Client")) {
          client.subscribe("image/hsv");
          client.publish("esp32/request", "{\"request\":\"state\"}");
          stateTimestamp = millis();
          state = WAITING_RESPONSE;
        }
      }
      break;

    case WAITING_RESPONSE:
      client.loop();

      if (responseReceived) {
        disconnectAll();
        responseReceived = false;
        state = DONE;
      }

      // timeout safety (5s)
      if (millis() - stateTimestamp > 5000) {
        Serial.println("Timeout");
        disconnectAll();
        state = DONE;
      }
      break;

    case DONE:
      // optional: deep sleep here
      state = IDLE;
      break;
  }
}

void sendStateRequest() {
  const char* request = "{\"request\":\"state\"}";
  client.publish("esp32/request/state", request);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, "image/hsv") != 0) return;

  StaticJsonDocument<16384> doc;
  DeserializationError err = deserializeJson(doc, payload, length);
  if (err) {
    Serial.println("JSON parse failed");
    return;
  }

  imageCount = 0;
  currentImage = 0;

  fps = doc["fps"] | 20;
  int brightness = doc["brightness"] | 255;
  FastLED.setBrightness(brightness);

  JsonArray images = doc["images"];
  for (JsonArray image : images) {
    if (imageCount >= MAX_IMAGES) break;

    for (int i = 0; i < NUM_LEDS; i++) {
      JsonArray hsv = image[i];
      pixelData[imageCount][i*3 + 0] = hsv[0]; // H
      pixelData[imageCount][i*3 + 1] = hsv[1]; // S
      pixelData[imageCount][i*3 + 2] = hsv[2]; // V
    }
    imageCount++;
  }

  Serial.printf("Received %d images\n", imageCount);
  responseReceived = true;
}

void connectWiFiAndMQTT() {
  Serial.println("Connecting WiFi...");
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  stateTimestamp = millis();
}

void disconnectAll() {
  client.disconnect();
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi OFF");
}

bool buttonPressed() {
  static bool lastState = HIGH;
  bool current = digitalRead(BUTTON_PIN);

  if (lastState == HIGH && current == LOW) {
    delay(20);  // debounce
    lastState = current;
    return true;
  }
  lastState = current;
  return false;
}

void updateLED() {
  if (imageCount == 0) return;

  unsigned long frameInterval = 1000 / fps;
  if (millis() - lastFrameTime < frameInterval) return;

  lastFrameTime = millis();

  for (int i = 0; i < NUM_LEDS; i++) {
    uint8_t h = pixelData[currentImage][i*3 + 0];
    uint8_t s = pixelData[currentImage][i*3 + 1];
    uint8_t v = pixelData[currentImage][i*3 + 2];
    leds[i] = CHSV(h, s, v);
  }

  FastLED.show();

  currentImage++;
  if (currentImage >= imageCount) {
    currentImage = 0;  // loop animation
  }
}

