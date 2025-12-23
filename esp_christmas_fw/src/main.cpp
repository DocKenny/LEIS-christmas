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

bool isLastPacket = false;
volatile bool requestNextPacket = false;

WiFiClient espClient;
PubSubClient client(espClient);

CRGB leds[NUM_LEDS];

uint8_t pixelData[BUFFER_COUNT][MAX_IMAGES][NUM_LEDS * 3];

DeviceState state = IDLE;

unsigned long stateTimestamp = 0;
bool responseReceived = false;

volatile uint8_t activeBuffer = 0;     // used by LED thread
volatile uint8_t writeBuffer  = 1;     // used by MQTT callback

volatile uint16_t imageCountBuf[BUFFER_COUNT] = {0, 0};
volatile uint16_t fpsBuf[BUFFER_COUNT] = {20, 20};

// Track which chunk we're on
volatile uint32_t currentChunk = 0;
volatile bool videoComplete = false;

SemaphoreHandle_t bufferMutex;

TaskHandle_t ledTaskHandle = nullptr;

bool buttonPressed();
void sendStateRequest();
void mqttCallback(char* topic, byte* payload, unsigned int length);
void connectWiFiAndMQTT();
void disconnectAll();
void swapBuffers();
void clearAllBuffers();
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
    40000,
    nullptr,
    2,
    nullptr,
    1 // Core 1 (Wi-Fi)
  );

  Serial.println("Setup complete");
}

void loop() {
  switch (state) {
    case IDLE:
      if (buttonPressed()) {
        clearAllBuffers();
        currentChunk = 0;
        isLastPacket = false;
        requestNextPacket = false;
        
        connectWiFiAndMQTT();
        state = CONNECTING;
        stateTimestamp = millis();
      }
      break;

    case CONNECTING:
      // Timeout after 10 seconds
      if (millis() - stateTimestamp > 10000) {
        Serial.println("Connection timeout");
        state = DONE;
      }
      
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
      // Timeout after 1 minute
      if (millis() - stateTimestamp > 60000) {
        Serial.println("Response timeout");
        state = DONE;
      }
      
      if (responseReceived) {
        if (!videoComplete) {
          sendStateRequest();
          stateTimestamp = millis(); // Reset timeout
        } else {
          state = DONE;
          Serial.println("Video complete, disconnecting...");
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
  char request[64];
  snprintf(request, sizeof(request), 
           "{\"request\":\"state\", \"chunk\":%lu}", currentChunk);
  
  Serial.print("Publishing to topic 'esp32/request/state': ");
  Serial.println(request);
  
  bool success = client.publish("esp32/request/state", request);
  if (success) {
    Serial.println("Publish successful");
  } else {
    Serial.println("Publish failed!");
  }
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
  isLastPacket = doc["isLastPacket"] | false;
  
  // Extract chunk info
  uint32_t chunkNumber = doc["chunk"] | 0;
  bool isFirstChunk = (chunkNumber == 0);
  
  int brightness = doc["brightness"] | 50;
  FastLED.setBrightness(brightness);

  JsonArray images = doc["images"];
  
  if (images.isNull()) {
    JsonArray singleImage = doc["image"];
    
    if (!singleImage.isNull()) {
      JsonArray tempImages;
      images = tempImages;
      images.add(singleImage);
    } else {
      Serial.println("No 'image' or 'images' found in JSON");
      return;
    }
  }

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
  
  Serial.print("Received chunk ");
  Serial.print(chunkNumber);
  Serial.print(" with ");
  Serial.print(imageCountBuf[buf]);
  Serial.println(" frames");

  if (imageCountBuf[buf] > 0) {
    uint8_t oldActive = activeBuffer;
    imageCountBuf[oldActive] = 0;
    
    swapBuffers();
    Serial.println("Swapped buffers after receiving first chunk");
  }
  
  if (isLastPacket) {
    videoComplete = true;
    Serial.println("Video complete - last packet received");
  } else {
    currentChunk++;
  }
  
  responseReceived = true;
}

void clearAllBuffers() {
  xSemaphoreTake(bufferMutex, portMAX_DELAY);
  
  // Clear both buffers
  imageCountBuf[0] = 0;
  imageCountBuf[1] = 0;
  
  // Reset buffer indices
  activeBuffer = 0;
  writeBuffer = 1;
  
  videoComplete = false;

  xSemaphoreGive(bufferMutex);
  
  Serial.println("All buffers cleared");
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
  
  Serial.println("Buffers swapped");
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
  static uint16_t currentImage = 0;
  unsigned long lastFrameTime = 0;

  for (;;) {
    uint8_t buf = activeBuffer;
    uint16_t count = imageCountBuf[buf];
    uint16_t fpsLocal = fpsBuf[buf];
    
    if (count > 0) {
      unsigned long frameInterval = 1000 / fpsLocal;

      if (millis() - lastFrameTime >= frameInterval) {
        lastFrameTime = millis();

        // Display current frame
        for (int i = 0; i < NUM_LEDS; i++) {
          uint8_t h = pixelData[buf][currentImage][i*3 + 0];
          uint8_t s = pixelData[buf][currentImage][i*3 + 1];
          uint8_t v = pixelData[buf][currentImage][i*3 + 2];
          leds[i] = CRGB(h, s, v);
        }

        FastLED.show();

        currentImage++;
        if (currentImage >= count) {
          currentImage = 0;
          
          // Check if other buffer has data
          uint8_t otherBuf = !buf;  // The other buffer
          if (imageCountBuf[otherBuf] > 0) {
            swapBuffers();
            currentImage = 0;  // Reset when swapping
            Serial.print("Swapped to buffer with ");
            Serial.print(imageCountBuf[otherBuf]);
            Serial.println(" frames");
          }
        }
      }
    } else {
      // If buffer is empty, reset currentImage
      currentImage = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}