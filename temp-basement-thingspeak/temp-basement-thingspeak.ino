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


WiFiClient espClient;
PubSubClient client(espClient);

DHT_Unified dht(DHTPIN, DHTTYPE);

int measureTime;

void setup() {

#ifdef DEBUG
    Serial.begin(115200);
    delay(10);

    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(WI_SSID);
#endif


/******* re-connect to wifi ***********/

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
#ifdef DEBUG          
        Serial.println("Something went wrong with DHT20 temperature...waiting for watchdog reset");
        Serial.println();
#endif
        while(1){}
        
    } else {
        temp = event.temperature;
#ifdef DEBUG            
        Serial.print("    Temperature: ");
        Serial.print(temp);
        Serial.println(" *C");
#endif            
    }

    dht.humidity().getEvent(&event);

    if (isnan(event.relative_humidity)) {
#ifdef DEBUG          
        Serial.println("Something went wrong with DHT20 humidity...waiting for watchdog reset");
        Serial.println();
#endif
        while(1){}
        
    } else {
        hum = event.relative_humidity;
#ifdef DEBUG
        Serial.print("    Humidity: ");
        Serial.print(hum);
        Serial.println("%");
#endif 
        }

/******* Send values **********/

    thingspeak_send(temp, hum, lum);
    mqtt_send(temp, hum, lum);


#ifdef DEBUG
    Serial.println("ESP8266 in sleep mode");
#endif

/******* entering in deep sleep **********/

    ESP.deepSleep(SLEEP * 1000000);
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
void thingspeak_send(float temp, float hum, unsigned int lum) {

    if ( espClient.connect(TH_SERVER, 80) ) {

#ifdef DEBUG
        Serial.println("Connecting to thingspeak server");
#endif

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
void mqtt_send(float temp, float hum, unsigned int lum) {

    client.setServer(MQTT_SERVER, 1883);

    if (!client.connected())
        mqtt_reconnect();

#ifdef DEBUG
    Serial.println("Connected to MQTT server");
    Serial.println();
#endif

    client.loop();
    client.publish(TEMPERATURE_TOPIC, String(temp).c_str(), true);
    client.publish(HUMIDITY_TOPIC, String(hum).c_str(), true);
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
    unsigned int val = analogRead(ANALOG_PIN);
    digitalWrite(COMPARE_PIN, LOW);

#ifdef DEBUG
    Serial.print("    Luminosity = ");
    Serial.print(map(val, 0, 1023, 0, 99));
    Serial.println();
#endif

    return map(val, 0, 1023, 0, 99);
}

