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
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>


#include "temp-indoor-thingspeak.h"
#include "ota.h"


#define BME_SCK 5
#define BME_SDI 4

void ICACHE_RAM_ATTR countPulse ();

Adafruit_BME280 bme;

int measureTime, geigerRefTime;
unsigned int counter;
unsigned long timerOTA;

void setup() {

    Serial.begin(115200);
    delay(10);
    Serial.println("\n");


/******* re-connect to wifi ***********/

    setup_wifi();

/******* OTA **********/
#ifdef OTA
    updateOTA();
    timerOTA = millis()+(SLEEP*1000); 
#endif


/******* Do the job **********/

    if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME280 sensor, check wiring!");
      while (1);
    }

    measureTime = millis();
    geigerRefTime = micros();

    attachInterrupt(D7, countPulse, FALLING);
}


void loop() {

    if( millis() > timerOTA ) {
        updateOTA();
        timerOTA = millis()+(SLEEP*1000); 

    }
    
    /* After deep sleep reset is activated by pin D0 (bridged to RST)
     * setup() is re-executed but loop() is not
     */
    if( millis() > measureTime ){

        measureTime = millis() + 60000;
        
        int elapsedTime = micros() - geigerRefTime;

        noInterrupts();
        unsigned int cpm = (float(counter)/elapsedTime)*60000000;
        counter = 0;
        geigerRefTime = micros();  //reset 
        interrupts();
        
        
        float temp = bme.readTemperature();
        float hum = bme.readHumidity();
        float pressure = bme.readPressure() / 100.0F;
        float alt = bme.readAltitude(1013.25);
  
        Serial.print("    Temp : ");
        Serial.print(temp);
        Serial.println(" °C");
        Serial.print("    Humidity : ");
        Serial.print(hum);
        Serial.println(" %");
        Serial.print("    Pressure : ");
        Serial.print(pressure);
        Serial.println(" hPa");
        Serial.print("    Alt : ");
        Serial.print(alt);
        Serial.println("m");
        Serial.print("    Geiger counter : ");
        Serial.print(cpm);
        Serial.println(" cpm");
        /******* Send values **********/

        mqtt_send(temp, hum, pressure, alt, cpm);

        thingspeak_send(temp, hum, pressure, alt, cpm);
     }
}

/** Interrupt (simple counter 
 *  
 */
 void countPulse(){

      counter++;
 }

