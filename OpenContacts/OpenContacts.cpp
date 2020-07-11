/* OpenContacts Firmware
 * Based on the OpenGarage Firmware
 * OpenContacts library
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

#include "OpenContacts.h"

byte  OpenContacts::state = OC_STATE_INITIAL;
File  OpenContacts::log_file;
byte  OpenContacts::led_reverse = 0;
byte  OpenContacts::dirty_bits = 0xFF;

static const char* config_fname = CONFIG_FNAME;
static const char* log_fname = LOG_FNAME;

extern OpenContacts oc;
/* Options name, default integer value, max value, default string value
 * Integer options don't have string value
 * String options don't have integer or max value
 */
OptionStruct OpenContacts::options[] = {
  {"fwv", OC_FWV,      255, ""},
  {"lsz", DEFAULT_LOG_SIZE,400,""},
  {"htp", 80,        65535, ""},
  {"mod", OC_MOD_AP,   255, ""},
  {"ssid", 0, 0, ""},  // string options have 0 max value
  {"pass", 0, 0, ""},
  {"name", 0, 0, DEFAULT_NAME},
  {"dns1", 0, 0, "8.8.8.8"},
  {"admin_name", 0, 0, DEFAULT_NAME},
  {"url", 0, 0, ""},
  {"bldg", 0, 0, ""},
  {"room", 0, 0, ""},
  {"occup", 1, 300, ""},
  {"use_buzz", 1, 1, ""},
  {"admin_read", 0, 1, ""},
  {"admin_api", 0, 0, DEFAULT_DKEY} 
};

void OpenContacts::begin() {
  digitalWrite(PIN_RESET, HIGH);
  pinMode(PIN_RESET, OUTPUT);
  
  ledcSetup(0, 5000, 8);
  digitalWrite(PIN_BUZZER, LOW);
  pinMode(PIN_BUZZER, OUTPUT);
  ledcAttachPin(PIN_BUZZER, LEDC_CHANNEL);

  analogSetAttenuation(ADC_2_5db);
  adcAttachPin(PIN_ADC);

  pinMode(PIN_LED, OUTPUT);
  set_led(LOW);
  
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  pinMode(PMOS, OUTPUT);
  digitalWrite(PMOS, HIGH);

  state = OC_STATE_INITIAL;
  SPIFFS.begin(true);
  if(!SPIFFS.begin()) {
    DEBUG_PRINTLN(F("failed to mount file system!"));
  }
}

void OpenContacts::options_setup() {
  int i;
  if(!SPIFFS.exists(config_fname)) { // if config file does not exist
    options_save(); // save default option values
    return;
  } 
  options_load();
  
  if(options[OPTION_FWV].ival != OC_FWV)  {
    // if firmware version has changed
    // re-save options, thus preserving
    // shared options with previous firmwares
    options[OPTION_FWV].ival = OC_FWV;
    options_save();
    return;
  }
}

void OpenContacts::options_reset() {
  DEBUG_PRINT(F("reset to factory default..."));
  if(!SPIFFS.remove(config_fname)) {
    DEBUG_PRINTLN(F("failed to remove config file"));
    return;
  }else{DEBUG_PRINTLN(F("Removed config file"));}
  DEBUG_PRINTLN(F("ok"));
}

void OpenContacts::log_reset() {
  if(!SPIFFS.remove(log_fname)) {
    DEBUG_PRINTLN(F("failed to remove log file"));
    return;
  }else{DEBUG_PRINTLN(F("Removed log file"));}
  DEBUG_PRINTLN(F("ok"));  
}

int OpenContacts::find_option(String name) {
  for(byte i=0;i<NUM_OPTIONS;i++) {
    if(name == options[i].name) {
      return i;
    }
  }
  return -1;
}

void OpenContacts::options_load() {
  File file = SPIFFS.open(config_fname, "r");
  DEBUG_PRINT(F("loading config file..."));
  if(!file) {
    DEBUG_PRINTLN(F("failed"));
    return;
  }
  byte nopts = 0;
  while(file.available()) {
    String name = file.readStringUntil(':');
    String sval = file.readStringUntil('\n');
    sval.trim();
    DEBUG_PRINT(name);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(sval);
    nopts++;
    if(nopts>NUM_OPTIONS+1) break;
    int idx = find_option(name);
    if(idx<0) continue;
    if(options[idx].max) {  // this is an integer option
      options[idx].ival = sval.toInt();
    } else {  // this is a string option
      options[idx].sval = sval;
    }
  }
  DEBUG_PRINTLN(F("ok"));
  file.close();
}

void OpenContacts::options_save() {
  File file = SPIFFS.open(config_fname, "w");
  DEBUG_PRINTLN(F("saving config file..."));  
  if(!file) {
    DEBUG_PRINTLN(F("failed"));
    return;
  }
  OptionStruct *o = options;
  for(byte i=0;i<NUM_OPTIONS;i++,o++) {
    file.print(o->name + ":");
    if(o->max)
      file.println(o->ival);
    else
      file.println(o->sval);
  }
  DEBUG_PRINTLN(F("ok"));  
  file.close();
  set_dirty_bit(DIRTY_BIT_JO, 1);
}

void OpenContacts::write_log(const LogStruct& data) {
  File file;
  uint curr = 0;
  DEBUG_PRINTLN(F("saving log data..."));  
  if(!SPIFFS.exists(log_fname)) {  // create log file
    file = SPIFFS.open(log_fname, "w");
    if(!file) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    // fill log file
    uint next = curr+1;
    file.write((const byte*)&next, sizeof(next));
    file.write((const byte*)&data, sizeof(LogStruct));
    LogStruct l;
    l.tstamp = 0;
    DEBUG_PRINTLN("Pre-filling log file");
    for(;next<MAX_LOG_SIZE;next++) {  // pre-fill the log file to maximum size
      file.write((const byte*)&l, sizeof(LogStruct));
    }
  } else {
    file = SPIFFS.open(log_fname, "r+");
    if(!file) {
      DEBUG_PRINTLN(F("failed"));
      return;
    }
    file.readBytes((char*)&curr, sizeof(curr));
    uint next = (curr+1) % options[OPTION_LSZ].ival;
    file.seek(0, SeekSet);
    file.write((const byte*)&next, sizeof(next));

    file.seek(sizeof(curr)+curr*sizeof(LogStruct), SeekSet);
    file.write((const byte*)&data, sizeof(LogStruct));
  }
  DEBUG_PRINTLN(F("ok"));      
  file.close();
  set_dirty_bit(DIRTY_BIT_JL, 1);
}

bool OpenContacts::read_log_start() {
  if(log_file) log_file.close();
  log_file = SPIFFS.open(log_fname, "r");
  if(!log_file) return false;
  uint curr;
  if(log_file.readBytes((char*)&curr, sizeof(curr)) != sizeof(curr)) return false;
  if(curr>=MAX_LOG_SIZE) return false;
  return true;
}

bool OpenContacts::read_log_next(LogStruct& data) {
  if(!log_file) return false;
  if(log_file.readBytes((char*)&data, sizeof(LogStruct)) != sizeof(LogStruct)) return false;
  return true;  
}

bool OpenContacts::read_log_end() {
  if(!log_file) return false;
  log_file.close();
  return true;
}

void OpenContacts::play_note(uint freq) {
  if (options[OPTION_BUZZ].ival) {
    if(freq>0) {
    ledcWriteTone(LEDC_CHANNEL, freq);
    } else {
      ledcWriteTone(LEDC_CHANNEL, 0);
    }
  }
}

void OpenContacts::play_multi_notes(uint number, uint del, uint freq) {
  if (options[OPTION_BUZZ].ival) { 
    for (uint i=0; i<number; i++) {
      ledcWriteTone(LEDC_CHANNEL, freq);
      delay(del);
      ledcWriteTone(LEDC_CHANNEL, 0);
    }
  }
}

#include "pitches.h"

void OpenContacts::play_startup_tune() {
  static uint melody[] = {NOTE_C4, NOTE_E4, NOTE_G4, NOTE_C5};
  static byte duration[] = {4, 8, 8, 8};
  
  for (byte i = 0; i < sizeof(melody)/sizeof(uint); i++) {
    uint delaytime = 1000/duration[i];
    play_note(melody[i]);
    delay(delaytime);
    play_note(0);
    delay(delaytime * 0.2);    // add 30% pause between notes
  }
}
