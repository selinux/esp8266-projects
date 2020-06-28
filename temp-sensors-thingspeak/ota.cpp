/*
 * =====================================================================================
 *
 *       Filename:  http_handler.cpp
 *
 *    Description:  PTL_ir_remote secrets (mqtt, wifi,...)
 *
 *        Version:  1.0
 *        Created:  04/07/2018 11:12:31
 *       Revision:  none
 *       Compiler:  gcc-avr
 *
 *         Author:  Sebastien Chassot (sinux), seba.ptl@sinux.net
 *        Company:  Post Tenebras Lab (Geneva's Hackerspace)
 *
 * =====================================================================================
 */
#include <ESP8266WiFi.h>
#include <DNSServer.h>            //Local DNS Server used for redirecting all requests to the configuration portal
#include <ESP8266WebServer.h>     //Local WebServer used to serve the configuration portal
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic

#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include "ThingSpeak.h"
#include <ArduinoJson.h>

#include <ESP8266httpUpdate.h>

#include "secrets.h"
#include "ota.h"


WiFiManager wifiManager;
WiFiClient espClient;
MDNSResponder mdns;

PubSubClient client(MQTT_SERVER, 1883, espClient);


unsigned long channelID = 142129;


/**
 * 
 */
bool updateOTA(){

    t_httpUpdate_return  ret = ESPhttpUpdate.update(FW_UPDATE_URL);

//    Serial.println(ESPhttpUpdate.getLastError());
    switch(ESPhttpUpdate.getLastError()) {
        case 11:
            Serial.println("OTA : Updating firmware...");
            break;
        case -11:
            Serial.println("OTA : esp unknown in PTL esp DB...");
            break;
        case -1:
            Serial.println("OTA : No server found");
            break;
        case -102:
            Serial.println("OTA : No update needed current firmware is up to date.");
            break;
        default:
           Serial.println("Unknown error");
    }
    return (ret == 11) ? true : false;
}


/**
 * 
 */
void setup_wifi() {
    delay(10);
    
    // We start by connecting to a WiFi network
    wifiManager.setTimeout(300);  // Time out after 5 mins.
    if (!wifiManager.autoConnect(WIFI_AP_NAME)) {
        Serial.print("Wifi failed to connect and hit timeout.");
        // Reboot. A.k.a. "Have you tried turning it Off and On again?"
        ESP.reset();
    }

    Serial.print("WiFi connected. IP address:\t");
    Serial.println(WiFi.localIP().toString());
    Serial.print("WiFi connected. MAC address:\t");
    Serial.println(WiFi.macAddress());

    if (mdns.begin(HOSTNAME, WiFi.localIP())) {
        Serial.println("MDNS responder started");
        Serial.println(HOSTNAME);
        Serial.print("IP address:\t");
        Serial.println(WiFi.localIP());           // Send the IP address of the ESP8266 to the computer
        Serial.println("\n");
    }
}


/** Send sensors values to Thinkspeak server
 *
 * @temp temperature value
 * @lum luminosity value
 */
void thingspeak_send(const float temp_out, const float temp, const float hum, const unsigned int lum, const float pres) {
  
    ThingSpeak.begin(espClient);
    ThingSpeak.setField( 1, String(temp_out).c_str());
    ThingSpeak.setField( 2, String(temp).c_str());
    ThingSpeak.setField( 3, String(hum).c_str());
    ThingSpeak.setField( 4, String(lum).c_str());
    ThingSpeak.setField( 5, String(pres).c_str());
   
    if (ThingSpeak.writeFields( channelID, TH_APIKEY ) ){
        Serial.println("Successfully sent values to ThingSpeak...");       
    }

//    ThingSpeak.writeField(channelID, 1, String(temp).c_str(), TH_APIKEY);

    espClient.stop();

}


/** Send sensors values to MQTT broker
 *
 * @temp temperature value
 * @lum luminosity value
 s
 */
void mqtt_send(const float temp_out, const float temp, const float hum, const unsigned int lum, const float pres) {

    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");

        if (client.connect(ESPID, MQTT_USER, MQTT_PASSWORD )) {
 
            Serial.println("connected");
            client.publish(OUT_TEMP_TOPIC, String(temp_out).c_str());
            client.publish(IN_TEMP_TOPIC, String(temp).c_str());
            client.publish(HUMIDITY_TOPIC, String(hum).c_str());
            client.publish(LIGHT_TOPIC, String(lum).c_str());
            client.publish(PRESSURE_TOPIC, String(pres).c_str());
#ifdef DOMOTICZ

            const int nb_sensors = 5;
            int idx[] = { 22, 20, 21, 24, 23};
            float svalue[] = { temp, temp, float(int(hum)), float(lum), pres/100 };
            char *dtype[] = { "Temp", "Temp", "Humidity", "General", "General"};
            char *stype[] = { "DS18B20", "DHT22", "DHT22", "Photores", "BME280"};

            for (int i = 0; i < 5;i++) {
                StaticJsonBuffer<1024> jsonBuffer;
                JsonObject& data = jsonBuffer.createObject();
                data["idx"] = idx[i];
                data["nvalue"] = (i == 2) ? svalue[i] : 0;
                data["svalue"] = String(svalue[i]);
                data["dtype"] = dtype[i];
                data["stype"] = stype[i];
                String output;
                data.printTo(output);
                client.publish(DOMOTICZ_TOPIC, output.c_str(), true);
                data.printTo(Serial);
            }
#endif   
        } else {
 
            Serial.print("failed with state ");
            Serial.print(client.state());
            Serial.println();
            delay(2000);
         }
    }
}


/**
 * 
 */
void resetWifi() {
    wifiManager.resetSettings();
}


/**
 * 
 */
void stopWifiUDP() {
     WiFiUDP::stopAll();
}
