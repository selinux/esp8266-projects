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
//#include <bsec.h>
#include <ESP8266WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include "temp-bme680-thingspeak.h"
#include "ota.h"
#include "secrets.h"

#define BME_SCK 5
#define BME_SDI 4

Adafruit_BME680 bme; // I2C
#define SEALEVELPRESSURE_HPA (1013.25)

unsigned long timerOTA;
unsigned long timerMeasure;

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

/******* entering in deep sleep **********/

    if(!isUpdating) {
        /******* Do the job **********/
        if (!bme.begin(0x76)) {
            Serial.println("Could not find a valid BME680 sensor, check wiring!");
            while (1);
        }

        // Set up oversampling and filter initialization
        bme.setTemperatureOversampling(BME680_OS_8X);
        bme.setHumidityOversampling(BME680_OS_2X);
        bme.setPressureOversampling(BME680_OS_4X);
        bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
        bme.setGasHeater(320, 150); // 320*C for 150 ms

    } else {
        Serial.println("Updating...");
    }

}


void loop() {

    if( millis() > timerOTA ) {
        Serial.println("Perform sensors measurement...");
        updateOTA();
        timerOTA = millis() + SLEEP_OTA*1000;

    }

    if( millis() > timerMeasure ) {

        timerMeasure = millis() + SLEEP_MEASURES*1000;
        Serial.println("Performing sensors measurement...");
        measure();
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

    Serial.println("Luminosity :\t");
    Serial.println(map(val, 0, 300, 0, 99));
    Serial.println("---------------------------------------------------");
    return map(val, 0, 300, 0, 99);
}


void measure() {

    float val[6];

    if (! bme.performReading()) {
        Serial.println("Failed to perform reading :(");
        return;
    }
    val[0] = bme.temperature;
    val[1] = bme.pressure/100.0;
    val[2] = bme.humidity;
    val[3] = bme.gas_resistance/1000.0;
    val[4] = bme.readAltitude(SEALEVELPRESSURE_HPA);
    val[5] = get_luminosity();

    Serial.print("Temperature = ");
    Serial.print(val[0]);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(val[1]);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(val[2]);
    Serial.println(" %");

    Serial.print("Gas = ");
    Serial.print(val[3]);
    Serial.println(" KOhms");

    Serial.print("Approx. Altitude = ");
    Serial.print(val[4]);
    Serial.println(" m");

    Serial.print("Luminosity = ");
    Serial.println(val[5]);

    Serial.println();



    /******* Send values **********/
    mqtt_send(val[0], val[1], val[2], val[3], val[4], val[5]);
    thingspeak_send(val[0], val[1], val[2], val[3], val[4], val[5]);

}