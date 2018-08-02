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
#include <PubSubClient.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
//#include <BME280.h>

#include "temp-indoor-thingspeak.h"
#include "secrets.h"
#include "ota.h"


WiFiClient espClient;
PubSubClient client(espClient);

Adafruit_BME280 bme;
bool metric = true;

int measureTime, geigerRefTime;
unsigned int counter;

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

//    while(!bme.begin()){
//        Serial.println("Could not find BME280 sensor!");
//        delay(1000);
//    }

    measureTime = millis();
    geigerRefTime = micros();

    attachInterrupt(D7, countPulse, FALLING);

}


void loop() {

    /* After deep sleep reset is activated by pin D0 (bridged to RST)
     * setup() is re-executed but loop() is not
     */
     if(millis() > measureTime){

        measureTime = millis() + SLEEP * 1000;
        
        int elapsedTime = micros() - geigerRefTime;

        noInterrupts();
        unsigned int cpm = (float(counter)/elapsedTime)*60000000;
        counter = 0;
        geigerRefTime = micros();  //reset 
        interrupts();
        
        
        float temp(NAN), hum(NAN), pressure;
        uint8_t pressureUnit(1);
        //bme.ReadData(pressure, temp, hum, metric, pressureUnit);                // Parameters: (float& pressure, float& temp, float& humidity, bool hPa = true, bool celsius = false)
        float alt = 10;//bme.readAltitude(1013.25);
  
        unsigned int lum = get_luminosity();
  
        Serial.print("    Temp : ");
        Serial.print(bme.readTemperature());
        Serial.println(" °C");
        Serial.print("    Pressure : ");
        Serial.print(bme.readPressure());
        Serial.println(" hPa");
        Serial.print("    Alt : ");
        //Serial.print(bme.readAltitude(1013.25));
        Serial.println("m");
        Serial.print("    Geiger counter : ");
        Serial.print(cpm);
        Serial.println(" cpm");
        /******* Send values **********/
  
        thingspeak_send(temp, pressure, alt, lum, cpm);
        mqtt_send(temp, pressure, alt, lum, cpm);
     }
}

/** Interrupt (simple counter 
 *  
 */
 void countPulse(){

      counter++;
 }

/** Send sensors values to Thinkspeak server
 *
 * @temp temperature value
 * @lum luminosity value
 */
void thingspeak_send(float temp, float pressure, float alt, unsigned int lum, float cpm) {

    if ( espClient.connect(TH_SERVER, 80) ) {

#ifdef DEBUG
        Serial.println("Connecting to thingspeak server");
#endif

        String postStr = TH_APIKEY;
        postStr +="&field1=";
        postStr += String(temp);
        postStr +="&field2=";
        postStr += String(pressure);
        postStr +="&field3=";
        postStr += String(alt);
        postStr +="&field4=";
        postStr += String(lum);
        postStr +="&field5=";
        postStr += String(cpm);
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
void mqtt_send(float temp, float pressure, float alt, unsigned int lum, float cpm) {

    client.setServer(MQTT_SERVER, 1883);

    if (!client.connected())
        mqtt_reconnect();

#ifdef DEBUG
    Serial.println("Connected to MQTT server");
    Serial.println();
#endif

    client.loop();
    client.publish(TEMPERATURE_TOPIC, String(temp).c_str(), true);
    client.publish(PRESSURE_TOPIC, String(pressure).c_str(), true);
    client.publish(ALTITUDE_TOPIC, String(alt).c_str(), true);
    client.publish(LIGHT_TOPIC, String(lum).c_str(), true);
    client.publish(GEIGER_TOPIC, String(cpm).c_str(), true);

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
    unsigned int val = analogRead(ANALOG_PIN);

    Serial.print("    Luminosity = ");
    Serial.print(map(val, 0, 1023, 0, 100));
    Serial.println();

    return map(val, 0, 1023, 0, 100);
}

void printBME280Data(Stream* client){
  float temp(NAN), hum(NAN), pres(NAN);
  uint8_t pressureUnit(3);   // unit: B000 = Pa, B001 = hPa, B010 = Hg, B011 = atm, B100 = bar, B101 = torr, B110 = N/m^2, B111 = psi
  //bme.ReadData(pres, temp, hum, metric, pressureUnit);                // Parameters: (float& pressure, float& temp, float& humidity, bool hPa = true, bool celsius = false)
  temp = bme.readTemperature();
  hum = 10.1;//bme.hum();
  pres = bme.readPressure();
  /* Alternatives to ReadData():
    float ReadTemperature(bool celsius = false);
    float ReadPressure(uint8_t unit = 0);
    float ReadHumidity();

    Keep in mind the temperature is used for humidity and
    pressure calculations. So it is more effcient to read
    temperature, humidity and pressure all together.
   */
  client->print("Temp: ");
  client->print(temp);
  client->print("°"+ String(metric ? 'C' :'F'));
  client->print("\t\tHumidity: ");
  client->print(hum);
  client->print("% RH");
  client->print("\t\tPressure: ");
  client->print(pres);
  client->print(" hPa");
}
void printBME280CalculatedData(Stream* client){
  float altitude = 100;//bme.CalculateAltitude(metric);
  float dewPoint = 10.1;//bme.CalculateDewPoint(metric);
  client->print("\t\tAltitude: ");
  client->print(altitude);
  client->print((metric ? "m" : "ft"));
  client->print("\t\tDew point: ");
  client->print(dewPoint);
  client->println("°"+ String(metric ? 'C' :'F'));

}

