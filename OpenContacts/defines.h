/* OpenContacts Firmware
 * Based on OpenGarage Firmware
 * OpenContacts macro defines and hardware pin assignments
 * Mar 2016 @ OpenContacts.io
 *
 * This file is part of the OpenContacts library
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see
 * <http://www.gnu.org/licenses/>.
 */

#ifndef _DEFINES_H
#define _DEFINES_H

/** Firmware version, hardware version, and maximal values */
#define OC_FWV    111   // Firmware version: 111 means 1.1.1

/** GPIO pins */
#define PIN_RELAY   15 //D8 on nodemcu
#define PIN_BUTTON  0 //reset button
#define PIN_TRIG    12 //D6 on nodemcu
#define PIN_ECHO    14 //D5 on nodemcu
#define PIN_LED      2
#define PIN_RESET   16
#define PIN_BUZZER  17
#define LEDC_CHANNEL 0

#define RFSDA_PIN   21
#define RFRST_PIN   22
#define PIN_PIR     GPIO_NUM_33
#define PIN_ADC     GPIO_NUM_15
#define PMOS         5

#define RB 100  // voltage divider bottom and top resistor values (Kohm)
#define RT 470

// Default device name
#define DEFAULT_NAME    "My OpenContacts"
// Default device key
#define DEFAULT_DKEY    "opendoor"
// Config file name
#define CONFIG_FNAME    "/config.dat"
// Log file name
#define LOG_FNAME       "/log.dat"

#define OC_MNT_CEILING  0x00
#define OC_MNT_SIDE     0x01
#define OC_SWITCH_LOW   0x02
#define OC_SWITCH_HIGH  0x03

#define OC_ALM_NONE     0x00
#define OC_ALM_5        0x01
#define OC_ALM_10       0x02

#define OC_TSN_NONE     0x00
#define OC_TSN_AM2320   0x01
#define OC_TSN_DHT11    0x02
#define OC_TSN_DHT22    0x03
#define OC_TSN_DS18B20  0x04

#define OC_MOD_AP       0xA9
#define OC_MOD_STA      0x2A

#define OC_AUTO_NONE    0x00
#define OC_AUTO_NOTIFY  0x01
#define OC_AUTO_CLOSE   0x02

//Automation Option C - Notify settings
#define OC_NOTIFY_NONE  0x00
#define OC_NOTIFY_DO    0x01
#define OC_NOTIFY_DC    0x02
#define OC_NOTIFY_VL    0x04
#define OC_NOTIFY_VA    0x08

#define OC_STATE_INITIAL        0
#define OC_STATE_CONNECTING     1
#define OC_STATE_CONNECTED      2
#define OC_STATE_TRY_CONNECT    3
#define OC_STATE_WAIT_RESTART   4
#define OC_STATE_RESET          9
#define OC_STATE_RFID           5
#define OC_STATE_START_CONNECT  6

enum {
  DIRTY_BIT_JC = 0,
  DIRTY_BIT_JO,
  DIRTY_BIT_JL
};

#define DEFAULT_LOG_SIZE    100
#define MAX_LOG_SIZE       500
#define ALARM_FREQ         1000
// door status histogram
// number of values (maximum is 8)
#define DOOR_STATUS_HIST_K  4
#define DOOR_STATUS_REMAIN_CLOSED 0
#define DOOR_STATUS_REMAIN_OPEN   1
#define DOOR_STATUS_JUST_OPENED   2
#define DOOR_STATUS_JUST_CLOSED   3
#define DOOR_STATUS_MIXED         4

typedef enum {
  OPTION_FWV = 0, // firmware version
  OPTION_LSZ,     // log size
  OPTION_HTP,     // http port
  OPTION_MOD,     // mode
  OPTION_SSID,    // wifi ssid
  OPTION_PASS,    // wifi password
  OPTION_NAME,    // device name
  OPTION_DNS1,		// dns1 IP
  OPTION_ANAM,    // admin_name
  OPTION_URL,     // url
  OPTION_BLDG,    // bldg
  OPTION_ROOM,    // room
  OPTION_OCCP,    // occup
  OPTION_BUZZ,    // buzzer
  OPTION_ARD,     // admin_read
  OPTION_AAPI,    // admin_api 
  NUM_OPTIONS     // number of options
} OC_OPTION_enum;

// if button is pressed for 1 seconds, report IP
#define BUTTON_REPORTIP_TIMEOUT 800
// if button is pressed for at least 3 seconds, reset to AP mode
#define BUTTON_APRESET_TIMEOUT 2500
// if button is pressed for at least 7 seconds, factory reset
#define BUTTON_FACRESET_TIMEOUT  6500

#define LED_FAST_BLINK 100
#define LED_SLOW_BLINK 500

#define TIME_SYNC_TIMEOUT  1800 //Issues connecting to MQTT can throw off the time function, sync more often

#define TMP_BUFFER_SIZE 100

/** Serial debug functions */
#define SERIAL_DEBUG
#define DEBUG_BEGIN(x)   { Serial.begin(x); }

#if defined(SERIAL_DEBUG)

  #define DEBUG_PRINT(x)   Serial.print(x)
  #define DEBUG_PRINTLN(x) Serial.println(x)

#else

  #define DEBUG_PRINT(x)   {}
  #define DEBUG_PRINTLN(x) {}

#endif

typedef unsigned char byte;
typedef unsigned long ulong;
typedef unsigned int  uint;

#endif  // _DEFINES_H
