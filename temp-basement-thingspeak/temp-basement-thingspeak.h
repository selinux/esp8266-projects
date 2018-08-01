/*
 * =====================================================================================
 *
 *       Filename:  temp-basement-thingspeak.h
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


#define ANALOG_PIN          A0
#define COMPARE_PIN         D6

#define DHTPIN              D4         // Pin which is connected to the DHT sensor.

#define SLEEP               180

#define DHTTYPE             DHT22     // DHT 22 (AM2302)

#define TEMPERATURE_TOPIC   "pub/sensors/temp/bathroom"
#define HUMIDITY_TOPIC      "pub/sensors/humidity/bathroom"
#define LIGHT_TOPIC         "pub/sensors/light/bathroom"


void mqtt_reconnect();

void thingspeak_send(float temp, float hum, unsigned int lum);

void mqtt_send(float temp, float hum, unsigned int lum);

unsigned int get_luminosity();

float get_temperature();

