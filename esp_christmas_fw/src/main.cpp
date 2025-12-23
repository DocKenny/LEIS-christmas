#include <Arduino.h>
#include <WiFi.h>

#define MQTT_MAX_PACKET_SIZE 16384  // 16 KB

#include <PubSubClient.h>
#include <FastLED.h>
#include <ArduinoJson.h>
#include <config.h>
#include <helper.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"

#define BUFFER_COUNT 2
#define MAX_IMAGES 200

#define LED_PIN 3
#define NUM_LEDS 64
#define BUTTON_PIN 5 // TODO: Choose the correct pin

bool isLastPacket = true;
volatile bool requestNextPacket = false;

WiFiClient espClient;
PubSubClient client(espClient);

CRGB leds[NUM_LEDS];

uint8_t pixelData[BUFFER_COUNT][MAX_IMAGES][NUM_LEDS * 3];

DeviceState state = IDLE;

uint16_t imageCount = 0;
uint16_t currentImage = 0;
uint16_t fps = 20;
unsigned long lastFrameTime = 0;

unsigned long stateTimestamp = 0;
bool responseReceived = false;

volatile uint8_t activeBuffer = 0;     // used by LED thread
volatile uint8_t writeBuffer  = 1;     // used by MQTT callback

volatile uint16_t imageCountBuf[BUFFER_COUNT] = {0, 0};
volatile uint16_t fpsBuf[BUFFER_COUNT] = {20, 20};

SemaphoreHandle_t bufferMutex;

TaskHandle_t ledTaskHandle = nullptr;

bool buttonPressed();

void sendStateRequest();

void mqttCallback(char* topic, byte* payload, unsigned int length);

void connectWiFiAndMQTT();

void disconnectAll();

void swapBuffers();

void ledTask(void* param);

void mqttLoopTask(void* param);

void setup() {
  Serial.begin(115200);

  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
  FastLED.clear(true);

  pinMode(BUTTON_PIN, INPUT_PULLUP);

  WiFi.mode(WIFI_OFF);

  client.setServer(BROKER_IP, BROKER_PORT);
  client.setCallback(mqttCallback);
  client.setBufferSize(MQTT_MAX_PACKET_SIZE);

  bufferMutex = xSemaphoreCreateMutex();

  xTaskCreatePinnedToCore(
    ledTask,
    "LED Task",
    20000,
    nullptr,
    2,
    &ledTaskHandle,
    0   // Core 0 (WiFi runs on core 1)
  );

  xTaskCreatePinnedToCore(
    mqttLoopTask,
    "MQTT Loop",
    10000,
    nullptr,
    2,
    nullptr,
    1 // Core 1 (Wi-Fi)
  );

  Serial.println("Setup complete");
}


void loop() {
  // if(WiFi.status() == WL_CONNECTED && client.connected()) {
  //       client.loop(); // safe, only one call
  // }
  switch (state) {
    case IDLE:
      if (buttonPressed()) {
        connectWiFiAndMQTT();
        state = CONNECTING;
      }
      break;

    case CONNECTING:
      if (WiFi.status() == WL_CONNECTED) {
        if (client.connect("ESP32Client")) {
          Serial.println("MQTT connected");
          client.subscribe("image/hsv");
          sendStateRequest();
          stateTimestamp = millis();
          state = WAITING_RESPONSE;
        }
      }
      break;

    case WAITING_RESPONSE:
      if (responseReceived) {

        if (requestNextPacket && !isLastPacket) {
          sendStateRequest();
          requestNextPacket = false;
        }

        if (isLastPacket) {
          state = DONE;
        }
        responseReceived = false;
      }
      break;

    case DONE:
      disconnectAll();
      state = IDLE;
      break;
  }
}


void sendStateRequest() {
  const char* request = "{\"request\":\"state\"}";
  client.publish("esp32/request/state", request);
}

void mqttCallback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  
  if (strcmp(topic, "image/hsv") != 0) return;

  char json[length + 1];
  memcpy(json, payload, length);
  json[length] = '\0';

  DynamicJsonDocument doc(16384);
  DeserializationError err = deserializeJson(doc, json);
  
  if (err) {
    Serial.print("JSON parse failed: ");
    Serial.println(err.c_str());
    return;
  }

  uint8_t buf = writeBuffer;
  imageCountBuf[buf] = 0;
  
  fpsBuf[buf] = doc["fps"] | 20;
  isLastPacket = doc["isLastPacket"] | true;
  
  int brightness = doc["brightness"] | 100;
  FastLED.setBrightness(brightness);

  // Try to get images (plural) first
  JsonArray images = doc["images"];
  
  if (images.isNull()) {
    // Fall back to single image (singular)
    JsonArray singleImage = doc["image"];
    
    if (!singleImage.isNull()) {
      // Convert single image to array of one image for consistency
      JsonArray tempImages;
      images = tempImages;
      images.add(singleImage);
    } else {
      Serial.println("No 'image' or 'images' found in JSON");
      return;
    }
  }

  // Now process images array (could have 1 or more images)
  for (JsonVariant imageVar : images) {
    if (imageCountBuf[buf] >= MAX_IMAGES) {
      Serial.println("Max images reached, skipping rest");
      break;
    }
    
    JsonArray image = imageVar.as<JsonArray>();
    
    if (image.isNull()) {
      Serial.println("Invalid image format");
      continue;
    }
    
    Serial.print("Processing image ");
    Serial.print(imageCountBuf[buf]);
    Serial.print(" with ");
    Serial.print(image.size());
    Serial.println(" pixels");
    
    // Process pixels
    for (int i = 0; i < NUM_LEDS && i < image.size(); i++) {
      JsonArray hsv = image[i].as<JsonArray>();
      
      if (hsv.size() >= 3) {
        pixelData[buf][imageCountBuf[buf]][i*3 + 0] = hsv[0].as<uint8_t>();
        pixelData[buf][imageCountBuf[buf]][i*3 + 1] = hsv[1].as<uint8_t>();
        pixelData[buf][imageCountBuf[buf]][i*3 + 2] = hsv[2].as<uint8_t>();
      }
    }
    
    imageCountBuf[buf]++;
  }
  
  Serial.print("Received ");
  Serial.print(imageCountBuf[buf]);
  Serial.println(" images");
  
  swapBuffers();
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

void swapBuffers() {
  xSemaphoreTake(bufferMutex, portMAX_DELAY);

  uint8_t tmp = activeBuffer;
  activeBuffer = writeBuffer;
  writeBuffer = tmp;

  xSemaphoreGive(bufferMutex);
}

void mqttLoopTask(void* param) {
    for (;;) {
        if(WiFi.status() == WL_CONNECTED && client.connected()) {
            client.loop();  // single-threaded access
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // yield to other tasks
    }
}


void ledTask(void* param) {
  uint16_t currentImage = 0;
  unsigned long lastFrameTime = 0;

  for (;;) {
    uint8_t buf = activeBuffer;
    uint16_t count = imageCountBuf[buf];
    uint16_t fpsLocal = fpsBuf[buf];
    //Serial.println("LED Task: ");
    if (count > 0) {
      unsigned long frameInterval = 1000 / fpsLocal;

      if (millis() - lastFrameTime >= frameInterval) {
        lastFrameTime = millis();

        for (int i = 0; i < NUM_LEDS; i++) {
          uint8_t h = pixelData[buf][currentImage][i*3 + 0];
          uint8_t s = pixelData[buf][currentImage][i*3 + 1];
          uint8_t v = pixelData[buf][currentImage][i*3 + 2];
          leds[i] = CHSV(h, s, v);
        }

        FastLED.show();

        currentImage++;
        if (currentImage >= count) {
          currentImage = 0;
          if (!isLastPacket) {
            requestNextPacket = true;
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1)); // yield
  }
}


