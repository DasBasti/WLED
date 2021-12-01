#pragma once

#include "wled.h"
#include "Arduino.h"
#include <deque>

#ifndef WLED_ENABLE_MQTT
#error "This user mod requires MQTT to be enabled."
#endif

#define USERMOD_ID_MQTT_BUZZER 901
#ifndef USERMOD_BUZZER_PIN
#define USERMOD_BUZZER_PIN GPIO_NUM_4
#endif
#define TONE_DURATION 200

/*
 * Usermods allow you to add own functionality to WLED more easily
 * See: https://github.com/Aircoookie/WLED/wiki/Add-own-functionality
 * 
 * Using a usermod:
 * 1. Copy the usermod into the sketch folder (same folder as wled00.ino)
 * 2. Register the usermod by adding #include "usermod_filename.h" in the top and registerUsermod(new MyUsermodClass()) in the bottom of usermods_list.cpp
 */

class MQTTBuzzerUsermod : public Usermod
{
private:
  unsigned long lastTime_ = 0;
  unsigned long delay_ = 0;
  std::deque<std::pair<uint32_t, unsigned long>> sequence_{};
  bool mqttSubscribed;
  uint32_t tones[10] = {880, 523, 554, 587, 622, 659, 698, 740, 783, 830 };
public:
  MQTTBuzzerUsermod() : mqttSubscribed(true)
  {
  }
  /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
  void setup()
  {
    // Setup the pin, and default to LOW
    pinMode(USERMOD_BUZZER_PIN, OUTPUT);
    //digitalWrite(USERMOD_BUZZER_PIN, LOW);

    ledcSetup(1, 4000, 8);

    ledcAttachPin(USERMOD_BUZZER_PIN, 1);

    // Beep on startup
    //sequence_.push_back({HIGH, 50});
    //sequence_.push_back({LOW, 0});
  }

  /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
  void connected()
  {
    // Double beep on WiFi
    sequence_.push_back({tones[1], 250});
    sequence_.push_back({tones[5], 50});
    sequence_.push_back({tones[3], 250});
    sequence_.push_back({tones[6], 50});
    sequence_.push_back({tones[8], 250});
    sequence_.push_back({tones[0], 150});
    sequence_.push_back({LOW, 0});
  }

  void onMqttConnect(bool sessionPresent)
  {
    mqtt->onMessage(
        std::bind(&MQTTBuzzerUsermod::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                  std::placeholders::_5, std::placeholders::_6));
    mqttSubscribed = false;
  }

  /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     */
  void loop()
  {
    if (sequence_.size() < 1)
      return; // Wait until there is a sequence
    if (millis() - lastTime_ <= delay_)
      return; // Wait until delay has elapsed

    auto event = sequence_.front();
    sequence_.pop_front();

    if (event.first == LOW)
    {
      ledcWrite(1, 0);
    }
    else
    {
      ledcWriteTone(1, event.first);
    }
    delay_ = event.second;

    lastTime_ = millis();

    if (!mqttSubscribed && mqtt && mqtt->connected())
    {
      char subuf[38];

      if (mqttDeviceTopic[0] != 0)
      {
        strcpy(subuf, mqttDeviceTopic);
        strcat_P(subuf, PSTR("/sound"));
        mqtt->subscribe(subuf, 0);
      }
      if (mqttGroupTopic[0] != 0)
      {
        strcpy(subuf, mqttGroupTopic);
        strcat_P(subuf, PSTR("/sound"));
        mqtt->subscribe(subuf, 0);
      }
      mqttSubscribed = true;
    }

  }

  /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
  uint16_t getId()
  {
    return USERMOD_ID_MQTT_BUZZER;
  }

  void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
};

inline void MQTTBuzzerUsermod::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total){

/* MSG: 128*[0-9] */
  if (len > 128)
    return;

  // 12  3 4

  unsigned long duration = TONE_DURATION;
  uint8_t note = 255;
  for(uint8_t i=0; i< len; i++){
    if(payload[i] == ' '){
      duration += TONE_DURATION;
      continue;
    }
    if(note != 255){
    sequence_.push_back({tones[note], duration});
    duration = TONE_DURATION;
    DEBUG_PRINTF("%d %d %d\n",payload[i], note, duration);
    }
    
    if (payload[i]<'0' || payload[i]>'9')
      continue;
    note = payload[i] - '0'; // shift to single digit int
  }
  sequence_.push_back({tones[note], duration});
  sequence_.push_back({LOW, 0});

}