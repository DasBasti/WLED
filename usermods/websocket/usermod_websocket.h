#pragma once

#include "wled.h"
#define FRAME_COUNT 2

enum artnet_state {SOF, HEADER, DATA};
typedef struct frame_buffer frame_buffer_t;
struct frame_buffer{
    e131_packet_t packet;
    frame_buffer_t *next_packet;
    bool processed;
};

class WebsocketUsermod : public Usermod {
  private:
    WiFiClient client;
    u8_t frame[10]; // Artnet frame
    u8_t header[10];
    char id[8] = {'A','r','t','-','N','e','t', 0};
    artnet_state state;
    u16_t cnt = 0;
    u16_t data_cnt = 0;
    int len;
    frame_buffer_t frames[FRAME_COUNT];
    frame_buffer_t *in_buffer;
    frame_buffer_t *out_buffer;
    u8_t lastId=0;
    u8_t numbers[255];
    u8_t numbers_counter=0;
public:
    //Functions called by WLED

    /*
     * setup() is called once at boot. WiFi is not yet connected at this point.
     * You can use it to initialize variables, sensors or similar.
     */
    void setup() {
        state = SOF;
        for(uint8_t i = 0; i < FRAME_COUNT; i++){
            memcpy(frames[i].packet.art_id , id, sizeof(id));
            frames[i].processed = true;
            if(i + 1 != FRAME_COUNT)
                frames[i].next_packet = &frames[i+1];
            else
                frames[i].next_packet = &frames[0];
        }
        in_buffer = &frames[1];
        out_buffer = &frames[0];
    }


    /*
     * connected() is called every time the WiFi is (re)connected
     * Use it to initialize network interfaces
     */
    void connected() {
      
    }


    uint32_t lastTime;
    /*
     * queueWorker() fetches frames from queue and displays them.
     */

    void queueWorker(){
        //static uint16_t miss;
        // is it time to show frame?
        uint32_t now = millis();
        if(now - lastTime > 40 && out_buffer->processed == false){
        handleE131Packet(&(out_buffer->packet), client.remoteIP(), P_ARTNET);
        out_buffer = out_buffer->next_packet;
        lastTime = now;
        //Serial.println(miss);
        //miss=0;
        //} else {
        //    miss++;
        }
    }

    /*
     * loop() is called continuously. Here you can check for events, read sensors, etc.
     * 
     * Tips:
     * 1. You can use "if (WLED_CONNECTED)" to check for a successful network connection.
     *    Additionally, "if (WLED_MQTT_CONNECTED)" is available to check for a connection to an MQTT broker.
     * 
     * 2. Try to avoid using the delay() function. NEVER use delays longer than 10 milliseconds.
     *    Instead, use a timer check as shown here.
     */
    void loop() {
        if (!WLED_CONNECTED)
            return;

        if (!client.connected() && !client.connect("platinenmacher.tech", 6454)) {
            return;
        } 
        
        while (client.available()>0) {
            len = client.read(frame, sizeof(frame));
            char c;
            e131_packet_t* packet = &(in_buffer->packet);
            for(int i=0; i<len; i++){
            
                c = frame[i];
                switch(state){
                case SOF:
                    // find start point                
                    if(c == packet->art_id[cnt]){
                        cnt++;
                        if(cnt == 8){
                            cnt = 0;
                            state = HEADER;
                            //Serial.println("Start OK");
                        }
                    } else {
                        cnt = 0;
                    }
                    break;

                case HEADER:
                    header[cnt] = c;
                    cnt++;
                    if(cnt == 10){
                        packet->art_opcode = (header[0]<<8) + (header[1]);
                        packet->art_protocol_ver = (header[2]<<8) + (header[3]);
                        packet->art_sequence_number = header[4];
                        numbers[numbers_counter++] = header[4];                        
                        packet->art_physical = header[5];
                        packet->art_universe = (header[6]<<8) + (header[7]);
                        packet->art_length = (header[8]<<8) + (header[9]);
                        cnt = 0;
                        state = DATA;
                        data_cnt = 0;
                        /*if(lastId == 255)
                            lastId=0;
                        if(lastId+1 != packet.art_sequence_number)
                            Serial.printf("c %d %d\n", lastId, packet.art_sequence_number);
                        */lastId = packet->art_sequence_number;
                    }
                    break;
                
                case DATA:
                    // cnt ist Länge der erwarteten bytes
                    packet->art_data[data_cnt] = c;
                    data_cnt++;
                    if (packet->art_length == data_cnt){
                        data_cnt = 0;
                        cnt = 0;
                        state = SOF;
                        //Serial.print("Data OK");
                        // TODO fix RT issue
                        //
                        in_buffer->processed = false;
                        in_buffer = in_buffer->next_packet;
                        //delay(1);
                        return;
                    }
                    break;

                default:
                    Serial.println("OOPS!");
                }
                
            }
            /*if(numbers_counter == 255){
                for(uint8_t i=0;i<155;i++)
                Serial.println(numbers[i]);
                numbers_counter =0;
            }*/
            queueWorker();
        }
    }

        /*
     * getId() allows you to optionally give your V2 usermod an unique ID (please define it in const.h!).
     * This could be used in the future for the system to determine whether your usermod is installed.
     */
    uint16_t getId()
    {
      return USERMOD_WEBSOCKET;
    }
};