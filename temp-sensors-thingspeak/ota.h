/*
 * =====================================================================================
 *
 *       Filename:  http_handler.h
 *
 *    Description:  PTL_ir_remote secrets (mqtt, wifi,...)
 *
 *        Version:  1.0
 *        Created:  04/07/2018 11:12:31
 *       Revision:  none
 *       Compiler:  gcc-avr
 *
 *         Author:  Sebastien Chassot (sinux), seba.ptl@sinux.net
 *        Company:  Post Tenebras Lab (Geneva's Hackerspace)
 *
 * =====================================================================================
 */

#ifndef __OTA_H__
#define __OTA_H__

#define OTA
#define ONEWIRE
#define DHTTEMP
#define BME280

#define ESPID           "esinux01"

#define FQDN(x)         x".local"
#define AP(x)           x"_rescue"

#define HOSTNAME        FQDN(ESPID)
#define WIFI_AP_NAME    AP(ESPID)

#define OUT_TEMP_TOPIC       "pub/sensors/temp_out/window"
#define IN_TEMP_TOPIC        "pub/sensors/temp/window"
#define HUMIDITY_TOPIC       "pub/sensors/humidity/window"
#define LIGHT_TOPIC          "pub/sensors/brightness/window"
#define PRESSURE_TOPIC       "pub/sensors/pressure/window"

#define FW_UPDATE_URL   "http://172.16.10.128:8080/firmware"

bool updateOTA();
void setup_wifi();
void thingspeak_send(const float temp_out, const float temp, const float hum, const unsigned int lum, const float pres);
void mqtt_send(const float temp_out, const float temp, const float hum, const unsigned int lum, const float pres);
void resetWifi();
void stopWifiUDP();

#endif //__OTA_H__
