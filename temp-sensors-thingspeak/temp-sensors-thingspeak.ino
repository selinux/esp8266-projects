/*
 * =====================================================================================
 *
 *       Filename:  temp-sensors-thingspeak.ino
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
#include <OneWire.h>
#include <PubSubClient.h>

#include "temp-sensors-thingspeak.h"
#include "secrets.h"


WiFiClient espClient;
PubSubClient client(espClient);

OneWire  ds(14);  // on pin 2 (a 4.7K resistor is necessary)

void setup() {

#ifdef DEBUG    

    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WI_SSID);
#endif


    WiFi.begin(WI_SSID, WI_PASSWD);
  
    while (WiFi.status() != WL_CONNECTED) {

#ifdef DEBUG
        Serial.print(".");
        delay(500);
#else
        delay(100);
#endif
    }

#ifdef DEBUG
    Serial.println("");
    Serial.println("WiFi connected");
#endif

}

void loop() {

    float temp = get_temperature();
    unsigned int lum = get_luminosity();

    thingspeak_send(temp, lum);
    mqtt_send(temp, lum);

#ifdef DEBUG
    Serial.println("ESP8266 in sleep mode");
#endif

    // delay(10000);
    ESP.deepSleep(SLEEP * 1000000);
    
}


/** Send sensors values to Thinkspeak server
 *
 * @temp temperature value
 * @lum luminosity value
 */
void thingspeak_send(float temp, unsigned int lum) {

    if ( espClient.connect(TH_SERVER, 80) ) {

#ifdef DEBUG
        Serial.println("Connecting to thingspeak server");
#endif

        String postStr = TH_APIKEY;
        postStr +="&field1=";
        postStr += String(temp);
        postStr +="&field2=";
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

#ifdef DEBUG
        Serial.println("Sensors values sent to thingspeak server");
        Serial.println();
#endif
    }

    espClient.stop();
}


/** Send sensors values to MQTT broker
 *
 * @temp temperature value
 * @lum luminosity value
 s
 */
void mqtt_send(float temp, unsigned int lum) {

    client.setServer(MQTT_SERVER, 1883);

    if (!client.connected())
        mqtt_reconnect();

#ifdef DEBUG
    Serial.println("Connected to MQTT server");
    Serial.println();
#endif

    client.loop();
    client.publish(TEMPERATURE_TOPIC, String(temp).c_str(), true);
    client.publish(LIGHT_TOPIC, String(lum).c_str(), true);

#ifdef DEBUG
    Serial.println("Sensors values sent to MQTT server");
    Serial.println();
#endif
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
    unsigned int analog_val = analogRead(A0);
    digitalWrite(COMPARE_PIN, LOW);

#ifdef DEBUG
    Serial.print("    Luminosity = ");
    Serial.print(analog_val);
    Serial.println();
#endif

    return map(analog_val, 0, 3000, 0, 100);
}


/** Get temperature from DS18B20
 *
 * @return sensor value (float)
 */
float get_temperature() {

    byte type_s;
    byte addr[8];
    byte data[12];
    byte present = 0;

    int i = 0;

    while ( !ds.search(addr) ) {
        ds.reset_search();
        delay(250);

#ifdef DEBUG
        Serial.println("One-wire sensore not found");
        Serial.println();
#endif
        if(++i > 10) {

#ifdef DEBUG
            Serial.println("Something wrong with temp sensor...waiting for watchdog reset");
            Serial.println();
#endif
            while(1){}
        }
    }

    ds.reset();
    ds.select(addr);
    ds.write(0x44, 1);       // start conversion, with parasite power on at the end
    delay(750);              // wait for acquisition 750ms is enough, maybe not

    present = ds.reset();
    ds.select(addr);
    ds.write(0xBE);                     // Read Scratchpad
    for ( int i = 0; i < 9; i++)        // we need 9 bytes
        data[i] = ds.read();


    // Convert the data to actual temperature
    int16_t raw = (data[1] << 8) | data[0];

    if (type_s) {

        raw = raw << 3; // 9 bit resolution default

        // "count remain" gives full 12 bit resolution
        if (data[7] == 0x10)
            raw = (raw & 0xFFF0) + 12 - data[6];

    } else {
        byte cfg = (data[4] & 0x60);
        // at lower res, the low bits are undefined, so let's zero them
        if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
        else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
        else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
        //// default is 12 bit resolution, 750 ms conversion time
    }

#ifdef DEBUG
    Serial.println("=======  Aquisition ========");
    Serial.println();
    Serial.print("    Temperature = ");
    Serial.print((float)raw / 16.0);
    Serial.println(" Celsius, ");
#endif

    return (float)raw / 16.0;
}
