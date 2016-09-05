/*
 * =====================================================================================
 *
 *       Filename:  temp-sensors-thingspeak.h
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
#define THERMO_PIN          14
#define DHTPIN              D4         // Pin which is connected to the DHT sensor.

#define SLEEP               60


#define TEMPERATURE_TOPIC   "pub/sensor_inside/temperature"
#define PRESSURE_TOPIC      "pub/sensor_inside/pressure"
#define ALTITUDE_TOPIC      "pub/sensor_inside/altitude"
#define LIGHT_TOPIC         "pub/sensor_inside/light"
#define GEIGER_TOPIC        "pub/sensor_inside/geiger"

#define DEBUG

#define MQTT_PORT           1883

void mqtt_reconnect();

void thingspeak_send(float temp_out, float temp_in, float hum, unsigned int lum, float cpm);

void mqtt_send(float temp_out, float temp_in, float hum, unsigned int lum, float cpm);

unsigned int get_luminosity();

float get_outside_temperature();

void printBME280Data(Stream * client);
void printBME280CalculatedData(Stream* client);
