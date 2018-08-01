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

#define HOSTNAME "esinux01.local"  // Name of the device you want in mDNS.
#define wifiSSID "esinux01_rescue"
#define FW_UPDATE_URL "http://172.16.10.128:8080/firmware"

bool updateOTA();
void setup_wifi();
void resetWifi();
void stopWifiUDP();

#endif //__OTA_H__
