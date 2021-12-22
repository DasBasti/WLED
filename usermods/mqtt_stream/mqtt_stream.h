#pragma once

#include "wled.h"
#ifndef WLED_ENABLE_MQTT
#error "This user mod requires MQTT to be enabled."
#endif

class UsermodMqttStream : public Usermod
{
private:
  bool mqttSubscribed;

public:
  UsermodMqttStream() : mqttSubscribed(true)
  {
  }

  void setup()
  {
  }

  void connected()
  {
  }

  void onMqttConnect(bool sessionPresent)
  {
    mqtt->onMessage(
          std::bind(&UsermodMqttStream::onMqttMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3, std::placeholders::_4,
                    std::placeholders::_5, std::placeholders::_6));
    mqttSubscribed = false;
  }

  void loop()
  {
    if (!mqttSubscribed && mqtt && mqtt->connected())
    {
      char subuf[38];

      if (mqttDeviceTopic[0] != 0)
      {
        strcpy(subuf, mqttDeviceTopic);
        strcat_P(subuf, PSTR("/stream"));
        mqtt->subscribe(subuf, 0);
      }
      if (mqttGroupTopic[0] != 0)
      {
        strcpy(subuf, mqttGroupTopic);
        strcat_P(subuf, PSTR("/stream"));
        mqtt->subscribe(subuf, 0);
      }
      mqttSubscribed = true;
    }
  }

  void onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total);
};

inline void UsermodMqttStream::onMqttMessage(char *topic, char *payload, AsyncMqttClientMessageProperties properties, size_t len, size_t index, size_t total)
{ // compare length of stream with led count
  if (strip.getLengthTotal() == len / 4)
  {
    realtimeLock(realtimeTimeoutMs * 5, REALTIME_MODE_GENERIC);
    for (int i = 0; i < len / 4; i++)
    {
      if (!arlsDisableGammaCorrection && strip.gammaCorrectCol)
      {
        strip.setPixelColor(i,
                            strip.gamma8(payload[i * 4]),
                            strip.gamma8(payload[(i * 4) + 1]),
                            strip.gamma8(payload[(i * 4) + 2]),
                            strip.gamma8(payload[(i * 4) + 3]));
      }
      else
      {
        strip.setPixelColor(i, payload[i * 4], payload[(i * 4) + 1], payload[(i * 4) + 2], payload[(i * 4) + 3]);
      }
    }
    strip.show();
  }
}
