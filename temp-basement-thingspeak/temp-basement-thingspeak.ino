/*
 * =====================================================================================
 *
 *       Filename:  temp-basement-thingspeak.ino
 *
 *    Description:  Temperature and brightness sensors on a ESP8266 who broadcast to
 *                  thingspeak server and MQTT broker
 *
 *        Version:  1.0
 *        Created:  04. 08. 2016 16:59:30
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  Sebastien Chassot (sinux), seba.ptl@sinux.net
 *        Company:
 *
 * =====================================================================================
 */
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <DHT_U.h>

#include "temp-basement-thingspeak.h"
#include "secrets.h"
#include "ota.h"



WiFiClient espClient;
PubSubClient client(espClient);

DHT_Unified dht(DHTPIN, DHTTYPE);

int measureTime;



void setup() {

    Serial.begin(115200);
    delay(10);
    Serial.println("\n");


/******* re-connect to wifi ***********/

    setup_wifi();


/******* OTA **********/
#ifdef OTA
    bool isUpdating = updateOTA();
#endif

/******* Do the job **********/

    unsigned int lum = get_luminosity();
    float temp = 0.0;
    float hum = 0.0;

    dht.begin();
    sensor_t sensor;
    dht.temperature().getSensor(&sensor);
    sensors_event_t event;  
    dht.temperature().getEvent(&event);

    if (isnan(event.temperature)) {
        Serial.println("Something went wrong with DHT22 temperature...waiting for watchdog reset");
        Serial.println();
        
    } else {
        temp = event.temperature;
        Serial.print("\tTemperature :\t");
        Serial.print(temp);
        Serial.println(" °C");
    }

    dht.humidity().getEvent(&event);

    if (isnan(event.relative_humidity)) {
        Serial.println("Something went wrong with DHT22 humidity...waiting for watchdog reset");
        Serial.println();
        
    } else {
        hum = event.relative_humidity;
        Serial.print("\tHumidity :\t");
        Serial.print(hum);
        Serial.println("%");
    }

/******* Send values **********/

    thingspeak_send(temp, hum, lum);

    client.setServer(MQTT_SERVER, 1883);
    
    client.loop();
    mqtt_send(temp, hum, lum);


/******* entering in deep sleep **********/

    if(!isUpdating) {
        ESP.deepSleep(SLEEP * 1000000);
        Serial.print(ESPID);
        Serial.println(" in sleep mode");

    } else {
        Serial.println("Updating...");
    }

}


void loop() {

    /* After deep sleep reset is activated by pin D0 (bridged to RST)
     * setup() is re-executed but loop() is not
     */
}


/** Send sensors values to Thinkspeak server
 *
 * @temp temperature value
 * @lum luminosity value
 */
void thingspeak_send(const float temp, const float hum, const unsigned int lum) {

    if ( espClient.connect(TH_SERVER, 80) ) {

        Serial.println("Connecting to thingspeak server");

        String postStr = TH_APIKEY;
        postStr +="&field1=";
        postStr += String(temp);
        postStr +="&field2=";
        postStr += String(hum);
        postStr +="&field3=";
        postStr += String(lum);
        postStr += "\r\n\r\n";

        espClient.print("POST /update HTTP/1.1\n");
        espClient.print("Host: api.thingspeak.com\n");
        espClient.print("Connection: close\n");
        espClient.print("X-THINGSPEAKAPIKEY: "+TH_APIKEY+"\n");
        espClient.print("Content-Type: application/x-www-form-urlencoded\n");
        espClient.print("Content-Length: ");
        espClient.print(postStr.length());
        espClient.print("\n\n");
        espClient.print(postStr);

        Serial.println("Sensors values sent to thingspeak server");
        Serial.println();
    }

   espClient.stop();
}


/** Send sensors values to MQTT broker
 *
 * @temp temperature value
 * @lum luminosity value
 s
 */
void mqtt_send(const float temp, const float hum, const unsigned int lum) {

//
//    bool res = client.connect(ESPID, MQTT_USER, MQTT_PASSWORD);
//    
    // Loop until we're reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");
        // Attempt to connect
        if (client.connect(ESPID, "selinux", "cooldump")) {
            Serial.println("connected");
            // Once connected, publish an announcement...
            client.publish("outTopic", "hello world");
            // ... and resubscribe
            client.subscribe("inTopic");
        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 5 seconds");
            // Wait 5 seconds before retrying
            delay(5000);
        }
    }


    Serial.println("Connected to MQTT server");
    Serial.println();
    Serial.print(TEMPERATURE_TOPIC);
    Serial.print("\t");
    Serial.println(String(temp).c_str());
    Serial.print(HUMIDITY_TOPIC);
    Serial.print("\t");
    Serial.println(String(hum).c_str());
    Serial.print(LIGHT_TOPIC);
    Serial.print("\t");
    Serial.println(String(lum).c_str());
    Serial.println("Sensors values sent to MQTT server");
    Serial.println();


    client.publish("test", "hello world esinux03");
    client.publish(TEMPERATURE_TOPIC, String(temp).c_str());
    client.publish(HUMIDITY_TOPIC, String(hum).c_str());
    client.publish(LIGHT_TOPIC, String(lum).c_str());

}


/** reconnect to MQTT broker
 *
 *  try 10 times and suicide
 */
void mqtt_reconnect() {

    // if not already connected Loop until reconnected
    while (!client.connected()) {
        Serial.print("Attempting MQTT connection...");

        if (client.connect("ESP8266Client", "selinux", "cooldump")) {
            Serial.println("connected");

        } else {
            Serial.print("failed, rc=");
            Serial.print(client.state());
            Serial.println(" try again in 2 seconds");
            // Wait 2 seconds before retrying
            delay(2000);
        }
    }
}


/** Get luminosity from photo-resistor
 *
 * @return a mapped value (0..100%)
 */
unsigned int get_luminosity() {

    pinMode(ANALOG_PIN, INPUT);
    pinMode(COMPARE_PIN, OUTPUT);
 
    // Compare pin act as a pull up
    digitalWrite(COMPARE_PIN, HIGH);
    delay(1);

    unsigned int val = analogRead(ANALOG_PIN);
    digitalWrite(COMPARE_PIN, LOW);

//    Serial.println(val);

    Serial.print("\tLuminosity :\t");
    Serial.print(map(val, 0, 199, 0, 99));
    Serial.println();

    return map(val, 0, 199, 0, 99);
}

