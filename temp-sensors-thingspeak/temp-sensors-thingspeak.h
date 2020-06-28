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

#define DEBUG

#define ANALOG_PIN          A0
#define SCK_PIN             D1
#define SDA_PIN             D2
#define THERMO_PIN          D6
#define DHT_PIN             D4         // Pin which is connected to the DHT sensor.

#define SLEEP               180

#define DHTTYPE             DHT22     // DHT 22 (AM2302)

#define MQTT_PORT           1883

#define SERIAL_BAUD 115200

unsigned int get_luminosity();
float get_pressure();

