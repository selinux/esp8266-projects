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

#define DOMOTICZ

#define ESPID           "esinux02"

#define FQDN(x)         x".home"
#define AP(x)           x"_rescue"

#define HOSTNAME        FQDN(ESPID)
#define WIFI_AP_NAME    AP(ESPID)

#define FW_UPDATE_URL   "http://172.16.10.128:8080/firmware"
//#define FW_UPDATE_URL "http://10.42.65.196:8080/firmware"

void updateOTA();
void setup_wifi();
void resetWifi();
void stopWifiUDP();


void thingspeak_send(float temp, float hum, float pres, float alt, float cpm);
void mqtt_send(float temp, float hum, float pres, float alt, float cpm);
void mqtt_reconnect();


#endif //__OTA_H__
