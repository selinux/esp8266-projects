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
#include <DNSServer.h> 
#include <WiFiManager.h>
#include <ESP8266httpUpdate.h>

#include <ESP8266mDNS.h>
#include <ArduinoJson.h>
#include <ThingSpeak.h>
#include <PubSubClient.h>

#include "ota.h"
#include "secrets.h"


#define TEMPERATURE_TOPIC   "pub/sensors/temp/kitchen"
#define HUMIDITY_TOPIC      "pub/sensors/humidity/kitchen"
#define PRESSURE_TOPIC      "pub/sensors/pressure/kitchen"
#define ALTITUDE_TOPIC      "pub/sensors/altitude/kitchen"
#define GEIGER_TOPIC        "pub/sensors/geiger/kitchen"
#define DOMOTICZ_TOPIC      "domoticz/in"


WiFiManager wifiManager;
MDNSResponder mdns;
WiFiClient espClient;
PubSubClient client(MQTT_SERVER, 1883, espClient);


/**
 * 
 */
void updateOTA(){

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


/** Send sensors values to Thinkspeak server
 *
 * @temp temperature value
 * @lum luminosity value
 */
void thingspeak_send(float temp, float hum, float pres, float alt, float cpm) {

    ThingSpeak.begin(espClient);
    ThingSpeak.setField( 1, String(temp).c_str());
    ThingSpeak.setField( 2, String(pres).c_str());
    ThingSpeak.setField( 3, String(alt).c_str());
    ThingSpeak.setField( 4, String(hum).c_str());
    ThingSpeak.setField( 5, String(cpm).c_str());

    if (ThingSpeak.writeFields( channelID, TH_APIKEY ) ){
        Serial.println();
        Serial.println("Successfully sent values to ThingSpeak...");
    }

    espClient.stop();

}


/** Send sensors values to MQTT broker
 *
 * @temp temperature value
 * @lum luminosity value
 s
 */
void mqtt_send(float temp, float hum, float pres, float alt, float cpm) {
    while (!client.connected()) {
        Serial.println("Connecting to MQTT...");

        if (client.connect("ESP8266Client", "selinux", "cooldump" )) {


            client.loop();
            client.publish(TEMPERATURE_TOPIC, String(temp).c_str(), true);
            client.publish(HUMIDITY_TOPIC, String(hum).c_str(), true);
            client.publish(PRESSURE_TOPIC, String(pres).c_str(), true);
            client.publish(ALTITUDE_TOPIC, String(alt).c_str(), true);
            client.publish(GEIGER_TOPIC, String(cpm).c_str(), true);

            Serial.println("Sensors values sent to MQTT server");
            Serial.println();


#ifdef DOMOTICZ
            const int nb_sensors = 5;
            int idx[] = { 57, 58, 59, 60, 61 };
            float svalue[] = { temp, int(hum), pres, alt, cpm };
            char *dtype[] = { "Temp", "Humidity", "Pressure", "Elevation", "Geiger"};
            char *stype[] = { "BME280", "BME280", "BME280", "BME280", "Geiger"};

            for (int i = 0; i < nb_sensors;i++) {
                StaticJsonDocument<1024> doc;
                JsonObject data = doc.to<JsonObject>();
                data["idx"] = idx[i];
                data["nvalue"] = ( i == 1 ) ? svalue[i] : 0;
                data["svalue"] = String(svalue[i]);
                data["dtype"] = dtype[i];
                data["stype"] = stype[i];
                String output;
                serializeJson(data, output);
                client.publish(DOMOTICZ_TOPIC, output.c_str(), true);
                serializeJson(data, Serial);
            }
            Serial.println("\nSensors values sent to domoticz server");
            Serial.println();
#endif

        } else {

            Serial.print("failed with state ");
            Serial.print(client.state());
            Serial.println();
            delay(2000);
        }
    }
}


/** reconnect to MQTT broker
 *
 *  try 10 times and suicide
 */
void mqtt_reconnect() {

    int i = 0;
    // if not already connected Loop until reconnected

    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (client.connect("ESP8266Client", MQTT_USER, MQTT_PASSWORD))
            Serial.println("connected");

        if (i++ > 10) {
#ifdef DEBUG
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println("Something wrong with MQTT server...waiting for watchdog reset");
            Serial.println();
#endif
            while(1){}
        }
    }
}

