/* OpenContacts Firmware
 * Based on the OpenGarage Firmware
 * OpenContacts library header file
 * July 2020 @ OpenContacts.io
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

#ifndef _OPENCONTACTS_H
#define _OPENCONTACTS_H

#include <Arduino.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <Ticker.h>

#include "defines.h"

struct OptionStruct {
  String name;
  uint ival;
  uint max;
  String sval;
};

struct LogStruct {
  long tstamp; // time stamp
  ulong card_uid;  // card_uid
  uint voltage;    // voltage reading
};

class OpenContacts {
public:
  static OptionStruct options[];
  static byte state;
  static byte alarm;
  static byte led_reverse;
  static byte dirty_bits;
  static void begin();
  static void options_setup();
  static void options_load();
  static void options_save();
  static void options_reset();
  static void restart() { esp_restart();} //digitalWrite(PIN_RESET, LOW); }
  static byte get_mode()   { return options[OPTION_MOD].ival; }
  static byte get_button() { return digitalRead(PIN_BUTTON); }
  static byte get_led()    { return led_reverse?(!digitalRead(PIN_LED)):digitalRead(PIN_LED); }
  static void set_led(byte status)   { digitalWrite(PIN_LED, led_reverse?(!status):status); }
  static void set_dirty_bit(byte bit, byte value) {
    if(value==0) dirty_bits &= ~(1<<bit);
    else dirty_bits |= (1<<bit);
  }
  static byte get_dirty_bit(byte bit) {
    return (dirty_bits >> bit) & 1;
  }
  static int find_option(String name);
  static void log_reset();
  static void write_log(const LogStruct& data);
  static bool read_log_start();
  static bool read_log_next(LogStruct& data);
  static bool read_log_end();
  static void play_note(uint freq);
  static void play_multi_notes(uint number, uint del, uint freq);
  static void reset_to_ap() {
    options[OPTION_MOD].ival = OC_MOD_AP;
    options_save();
    restart();
  }
  static void play_startup_tune();
private:
  static ulong read_distance_once();
  static File log_file;
  static void button_handler();
  static void led_handler();
  
};

#endif  // _OPENCONTACTS_H_
