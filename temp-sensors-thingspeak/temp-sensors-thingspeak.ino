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
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Wire.h> 
#include "DHTesp.h"
#include <OneWire.h>

#include "temp-sensors-thingspeak.h"
#include "secrets.h"
#include "ota.h"

DHTesp dht;

OneWire  ds(THERMO_PIN);  // on pin 2 (a 4.7K resistor is necessary)

Adafruit_BME280 bme;


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

    // BME280
    if (!bme.begin(0x76)) {
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }

    // ONE WIRE
    float temp_out = get_outside_temperature();

    // Brightness
    unsigned int lum = get_luminosity();


    float temp = bme.readTemperature();
    float hum = bme.readHumidity();
    float pres = bme.readPressure() / 100.0F;
    float alt = bme.readAltitude(1013.25);

    Serial.print("    Temp : ");
    Serial.print(temp);
    Serial.println(" °C");
    Serial.print("    Humidity : ");
    Serial.print(hum);
    Serial.println(" %");
    Serial.print("    Pressure : ");
    Serial.print(pres);
    Serial.println(" hPa");
    Serial.print("    Alt : ");
    Serial.print(alt);
    Serial.println("m");

//    float pres = get_pressure();

/******* Send values **********/

    mqtt_send(temp_out, temp, hum, lum, pres);

    thingspeak_send(temp_out, temp, hum, lum, pres);


/******* entering in deep sleep **********/
//    unsigned long pause = millis() + 4000;
//    Serial.println("Wait a while...");
//    while(millis() < pause) {;}

    delay(2000);

    if(!isUpdating) {
        Serial.print(ESPID);
        Serial.println(" in sleep mode");
        ESP.deepSleep(SLEEP * 1000000);

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
    unsigned int val = analogRead(ANALOG_PIN);

    Serial.println("Luminosity :\t");
    Serial.println(map(val, 0, 1023, 0, 99));
    Serial.println("---------------------------------------------------");

    return map(val, 0, 1023, 0, 100);
}


/** Get temperature from DS18B20
 *
 * @return sensor value (float)
 */
float get_outside_temperature() {

    byte type_s;
    byte addr[8];
    byte data[12];
    byte present = 0;

    int i = 0;


    while ( !ds.search(addr) ) {
        ds.reset_search();
        delay(250);


        Serial.println("One-wire sensore not found");
        Serial.println();

        if(++i > 10) {

#ifdef DEBUG
            Serial.println("Something wrong with temp sensor...waiting for watchdog reset");
            Serial.println();
#endif
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
