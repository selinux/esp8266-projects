/*
 * =====================================================================================
 *
 *       Filename:  temp-bme680-thingspeak.h
 *
 *    Description:  Temperature and brightness sensors on a ESP8266 who broadcast to 
 *                  thingspeak server and MQTT broker
 *
 *        Version:  1.0
 *        Created:  27. 06. 2020 11:39:30
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
#define SCK_PIN             D1
#define SDA_PIN             D2

#define SLEEP_OTA           600
#define SLEEP_MEASURES      180


unsigned int get_luminosity();
void measure();
