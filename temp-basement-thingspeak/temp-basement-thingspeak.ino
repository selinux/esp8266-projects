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
#include "DHTesp.h"

#include "temp-basement-thingspeak.h"
#include "ota.h"
#include "secrets.h"

DHTesp dht;



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


    dht.setup(DHT_PIN, DHTesp::DHT22); // Connect DHT sensor to GPIO 17

    delay(dht.getMinimumSamplingPeriod());

    float temp = dht.getTemperature();
    float hum = dht.getHumidity();

    Serial.println("state\ttemp\t\thumidity\ttemp avg");
    Serial.println("---------------------------------------------------");
    Serial.print(dht.getStatusString());
    Serial.print("\t");
    Serial.print(temp, 1);
    Serial.print(" °C\t\t");
    Serial.print(hum, 1);
    Serial.print(" %\t\t");
    Serial.print(dht.computeHeatIndex(temp, hum, false), 1);
    Serial.println(" °C");
    Serial.println("---------------------------------------------------");

    unsigned int lum = get_luminosity();


/******* Send values **********/

    mqtt_send(temp, hum, lum);

    thingspeak_send(temp, hum, lum);



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

    Serial.println("Luminosity :\t");
    Serial.println(map(val, 0, 199, 0, 99));
    Serial.println("---------------------------------------------------");
    return map(val, 0, 199, 0, 99);
}

