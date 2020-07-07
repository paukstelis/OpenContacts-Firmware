/* OpenContacts Firmware
 * Based on the OpenGarage firmware
 * Main loop
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

#include <DNSServer.h>

#include "pitches.h"
#include "OpenContacts.h"
#include "espconnect.h"
#include "Update.h"
//OpenContacts
#include <MFRC522.h>
#include <SPI.h>
#include "driver/adc.h"

MFRC522::MIFARE_Key key;
MFRC522::StatusCode status;
MFRC522 mfrc522(RFSDA_PIN, RFRST_PIN);

OpenContacts oc;
WebServer *server = NULL;
DNSServer *dns = NULL;

//static Ticker led_ticker;
//static Ticker aux_ticker;
//static Ticker ip_ticker;
static Ticker restart_ticker;
RTC_DATA_ATTR time_t now;
static WiFiClient wificlient;

static String scanned_ssids;
static ulong cardKey = 0;
static ulong lastKey = 0;
static long voltage = 0;
static int volt_reads = 0;
static uint led_blink_ms = LED_FAST_BLINK;
static uint adc_timeout = 100;
static byte curr_mode;
static ulong curr_utc_time = 0;
static ulong curr_utc_hour= 0;
static HTTPClient http;
static bool send_logs = false;
void do_setup();
void do_wake();

byte findKeyVal (const char *str, const char *key, char *strbuf=NULL, uint8_t maxlen=0) {
  uint8_t found=0;
  uint8_t i=0;
  const char *kp;
  kp=key;
  while(*str &&  *str!=' ' && *str!='\n' && found==0){
    if (*str == *kp){
      kp++;
      if (*kp == '\0'){
        str++;
        kp=key;
        if (*str == '='){
            found=1;
        }
      }
    } else {
      kp=key;
    }
    str++;
  }
  if(strbuf==NULL) return found; // if output buffer not provided, return right away

  if (found==1){
    // copy the value to a buffer and terminate it with '\0'
    while(*str &&  *str!=' ' && *str!='\n' && *str!='&' && i<maxlen-1){
      *strbuf=*str;
      i++;
      str++;
      strbuf++;
    }
    if (!(*str) ||  *str == ' ' || *str == '\n' || *str == '&') {
      *strbuf = '\0';
    } else {
      found = 0;  // Ignore partial values i.e. value length is larger than maxlen
      i = 0;
    }
  }
  return(i); // return the length of the value
}

void server_send_html_P(PGM_P content) {
  server->send_P(200, PSTR("text/html"), content);
  DEBUG_PRINT(strlen_P(content));
  DEBUG_PRINTLN(F(" bytes sent."));
}

void server_send_json(String json) {
  server->sendHeader("Access-Control-Allow-Origin", "*"); // from esp8266 2.4 this has to be sent explicitly
  server->send(200, "application/json", json);
}

void server_send_result(byte code, const char* item = NULL) {
  String json = F("{\"result\":");
  json += code;
  if (!item) item = "";
  json += F(",\"item\":\"");
  json += item;
  json += F("\"");
  json += F("}");
  server_send_json(json);
}

void server_send_result(const char* command, byte code, const char* item = NULL) {
  if(!command) server_send_result(code, item);
}

bool get_value_by_key(const char* key, uint& val) {
  if(server->hasArg(key)) {
    val = server->arg(key).toInt();   
    return true;
  } else {
    return false;
  }
}

bool get_value_by_key(const char* key, String& val) {
  if(server->hasArg(key)) {
    val = server->arg(key);   
    return true;
  } else {
    return false;
  }
}

bool findArg(const char *command, const char *name) {
  if(command) {
    return findKeyVal(command, name);
    // todo
  } else {
    return server->hasArg(name);
  }
}

char tmp_buffer[TMP_BUFFER_SIZE];

bool get_value_by_key(const char *command, const char *key, uint& val) {
  if(command) {
    byte ret = findKeyVal(command, key, tmp_buffer, TMP_BUFFER_SIZE);
    val = String(tmp_buffer).toInt();
    return ret;
  } else {
    return get_value_by_key(key, val);
  }
}

bool get_value_by_key(const char *command, const char *key, String& val) {
  if(command) {
    byte ret = findKeyVal(command, key, tmp_buffer, TMP_BUFFER_SIZE);
    val = String(tmp_buffer);
    return ret;
  } else {
    return get_value_by_key(key, val);
  }
}

void restart_in(uint32_t ms) {
  if(oc.state != OC_STATE_WAIT_RESTART) {
    oc.state = OC_STATE_WAIT_RESTART;
    DEBUG_PRINTLN(F("Prepare to restart..."));
    restart_ticker.once_ms(ms, oc.restart);
  }
}

void on_home()
{
  if(curr_mode == OC_MOD_AP) {
    server_send_html_P(ap_home_html);
  } else {
    server_send_html_P(sta_home_html);
  }
}

char dec2hexchar(byte dec) {
  if(dec<10) return '0'+dec;
  else return 'A'+(dec-10);
}

String get_mac() {
  static String hex = "";
  if(!hex.length()) {
    byte mac[6];
    WiFi.macAddress(mac);

    for(byte i=0;i<6;i++) {
      hex += dec2hexchar((mac[i]>>4)&0x0F);
      hex += dec2hexchar(mac[i]&0x0F);
      if(i!=5) hex += ":";
    }
  }
  return hex;
}

String get_ap_ssid() {
  static String ap_ssid = "";
  if(!ap_ssid.length()) {
    byte mac[6];
    WiFi.macAddress(mac);
    ap_ssid = "OC_";
    for(byte i=3;i<6;i++) {
      ap_ssid += dec2hexchar((mac[i]>>4)&0x0F);
      ap_ssid += dec2hexchar(mac[i]&0x0F);
    }
  }
  return ap_ssid;
}

String get_ip() {
  String ip = "";
  IPAddress _ip = WiFi.localIP();
  ip = _ip[0];
  ip += ".";
  ip += _ip[1];
  ip += ".";
  ip += _ip[2];
  ip += ".";
  ip += _ip[3];
  return ip;
}

void on_reset_all(){
  oc.state = OC_STATE_RESET;
  server_send_result(HTML_SUCCESS);
}

void debug_log() {
  LogStruct l;
  if (oc.read_log_start()) {
    for(uint i=0;i<oc.options[OPTION_LSZ].ival;i++) {
      if(!oc.read_log_next(l)) break;
      if(!l.tstamp) continue;
      DEBUG_PRINTLN(l.tstamp);
    }
  oc.read_log_end();
  }
}

void log_data() {
    LogStruct l;
    l.tstamp = time(nullptr);
    l.card_uid = cardKey;
    l.voltage = voltage;
    oc.write_log(l);
    DEBUG_PRINTLN("Log written");
    DEBUG_PRINTLN(l.tstamp);
    DEBUG_PRINTLN(l.card_uid);
    DEBUG_PRINTLN(l.voltage);
    debug_log();
}

void on_clear_log() {
  oc.log_reset(); 
}

bool readCard() {
  if ( ! mfrc522.PICC_IsNewCardPresent()) {
    return false;
  }
  if ( ! mfrc522.PICC_ReadCardSerial()) {
    return false;
  }

  cardKey =  mfrc522.uid.uidByte[0] << 24;
  cardKey += mfrc522.uid.uidByte[1] << 16;
  cardKey += mfrc522.uid.uidByte[2] <<  8;
  cardKey += mfrc522.uid.uidByte[3];
  mfrc522.PICC_HaltA(); 
  mfrc522.PCD_StopCrypto1();

  if (cardKey == lastKey) {
    DEBUG_PRINTLN("Duplicate key read, not sending");
    cardKey=0;
    return false;
  }

  lastKey = cardKey;
  DEBUG_PRINTLN(cardKey);
  return true;
}

void sendLog() {
  static String record = "/php/record.php";
  String html_url, html_content;
  static HTTPClient http;
  struct LogStruct l;
  html_url = oc.options[OPTION_URL].sval+record;
  if (oc.read_log_start()) {
    http.begin(html_url);
    for(uint i=0;i<oc.options[OPTION_LSZ].ival;i++) {
      if(!oc.read_log_next(l)) break;
      if(!l.tstamp) continue;
      html_content = "uid="+String(l.card_uid)+"&mac="+get_mac()+"&voltage="+String(l.voltage)+"&time="+String(l.tstamp);
      http.addHeader("Content-Type", "application/x-www-form-urlencoded");
      DEBUG_PRINTLN(html_content);
      int httpCode = http.POST(html_content);
    }
    http.end();
    oc.read_log_end();
    oc.log_reset();
  }

  send_logs = false;
}

void sendData() {
  static String newdevice = "/php/newdevice.php";
  static String report = "/php/report.php";
  static String record = "/php/record.php";
  String html_url, html_content;
  static HTTPClient http;

  if (oc.get_mode() == OC_MOD_AP) {
    html_url = oc.options[OPTION_URL].sval+newdevice;
    html_content = "bldg="+oc.options[OPTION_BLDG].sval+
                   "&room="+oc.options[OPTION_ROOM].sval+"&admin="+oc.options[OPTION_ARD].ival+
                   "&api="+oc.options[OPTION_AAPI].sval+"&name="+oc.options[OPTION_ANAM].sval+
                   "&occup="+oc.options[OPTION_OCCP].ival+"&mac="+get_mac();
    DEBUG_PRINTLN("Sending new device information...");
  } else if (oc.options[OPTION_ARD].ival == 1 && cardKey) {
    html_url = oc.options[OPTION_URL].sval+report;
    html_content = "uid="+String(cardKey)+"&api="+oc.options[OPTION_AAPI].sval+"&name="+oc.options[OPTION_ANAM].sval+
                   "&mac="+get_mac();
    DEBUG_PRINTLN("Sending card information");
  } else if (cardKey) {
    html_url = oc.options[OPTION_URL].sval+record;
    html_content = "uid="+String(cardKey)+"&mac="+get_mac()+"&voltage="+String(voltage);
    DEBUG_PRINTLN("Sending record data....");
  }
  
  http.begin(html_url);
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  int httpCode = http.POST(html_content);
  //some callback for response code?
  if (httpCode == 200) {
    oc.play_multi_notes(1,80, 400);
    oc.play_multi_notes(1,80, 900);
    oc.play_multi_notes(1,80,1800);
    send_logs = true; //we know we have connection to server, so send any stored swipes
  } else {
    //log item
    log_data();
  }
  
  http.end();
  //reset cardKey
  cardKey = 0;

}

void time_keeping() {
  struct tm now;
  DEBUG_PRINTLN(F("Set time server"));
  configTime(0, 0, "time1.google.com", "pool.ntp.org", NULL);
  DEBUG_PRINTLN(getLocalTime(&now));
}

void on_ap_scan() {
  if(curr_mode == OC_MOD_STA) return;
  server_send_json(scanned_ssids);
}

void on_ap_mac() {
  if(curr_mode == OC_MOD_STA) return;
  String json = "";
  json += F("{");
  json += F("\"mac\":\"");
  json += get_mac();
  json += F("\"}");
  server_send_json(json);
}

void on_ap_change_config() {
  if(curr_mode == OC_MOD_STA) return;
  if(server->hasArg("ssid")&&server->arg("ssid").length()!=0) {
    oc.options[OPTION_SSID].sval = server->arg("ssid");
    oc.options[OPTION_PASS].sval = server->arg("pass");
    oc.options[OPTION_URL].sval = server->arg("url");
    oc.options[OPTION_BLDG].sval = server->arg("bldg");
    oc.options[OPTION_ROOM].sval = server->arg("room");
    oc.options[OPTION_BUZZ].ival = server->arg("buzzer").toInt();
    oc.options[OPTION_ANAM].sval = server->arg("admin_name");
    oc.options[OPTION_ARD].ival = server->arg("admin_read").toInt();
    oc.options[OPTION_AAPI].sval = server->arg("admin_api");
    oc.options[OPTION_OCCP].ival = server->arg("occup").toInt();

    oc.options_save();
    server_send_result(HTML_SUCCESS);
    oc.state = OC_STATE_TRY_CONNECT;

  } else {
    server_send_result(HTML_DATA_MISSING, "ssid");
  }
}

void on_ap_try_connect() {
  if(curr_mode == OC_MOD_STA) return;
  String json = "{";
  json += F("\"ip\":");
  json += (WiFi.status() == WL_CONNECTED) ? (uint32_t)WiFi.localIP() : 0;
  json += F("}");
  server_send_json(json);
  if(WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
    DEBUG_PRINTLN(F("IP received by client, restart."));
    restart_in(1000);
  }
}

void on_ap_debug() {
  String json = "";
  json += F("{");
  json += F(",\"fwv\":");
  json += oc.options[OPTION_FWV].ival;
  json += F("}");
  server_send_json(json);
}

void do_sleep() {
  DEBUG_PRINTLN("Entering deepsleep");
  digitalWrite(PMOS, HIGH);
  if(WiFi.status() == WL_CONNECTED) {
    //reset time
    time_keeping();
  }
  adc_power_off();
  esp_deep_sleep_start();
}

void do_wake() {
  digitalWrite(PMOS, LOW);
  SPI.begin();
  oc.play_multi_notes(4, 80, 800); // buzzer on
  volt_reads = 0; //
  mfrc522.PCD_Init();
}

void do_setup()
{
  DEBUG_BEGIN(115200);
  WiFi.mode(WIFI_OFF); // turn off persistent, fixing flash crashing issue
  oc.begin();
  oc.options_setup();
  curr_mode = oc.get_mode();
  if (curr_mode != OC_MOD_AP) {
    do_wake();
    esp_sleep_enable_ext0_wakeup(PIN_PIR, 1);
  }
  DEBUG_PRINT(F("Compile Info: "));
  DEBUG_PRINT(F(__DATE__));
  DEBUG_PRINT(F(" "));
  DEBUG_PRINTLN(F(__TIME__));
  //only start a server in AP mode
  if(!server && curr_mode == OC_MOD_AP) {
    server = new WebServer(oc.options[OPTION_HTP].ival);
    if(curr_mode == OC_MOD_AP) dns = new DNSServer();
    DEBUG_PRINT(F("server started @ "));
    DEBUG_PRINTLN(oc.options[OPTION_HTP].ival);
    oc.set_led(HIGH);
  }
  led_blink_ms = LED_FAST_BLINK;
  
}

void process_ui()
{
  // process button
  static ulong button_down_time = 0;
  if(oc.get_button() == LOW) {
    if(!button_down_time) {
      button_down_time = millis();
    } else {
      ulong curr = millis();
      if(curr > button_down_time + BUTTON_FACRESET_TIMEOUT) {
        led_blink_ms = 0;
        oc.set_led(LOW);
      } else if(curr > button_down_time + BUTTON_APRESET_TIMEOUT) {
        led_blink_ms = 0;
        oc.set_led(HIGH);
      }
    }
  }
  else {
    if (button_down_time > 0) {
      ulong curr = millis();
      if(curr > button_down_time + BUTTON_FACRESET_TIMEOUT) {
        oc.state = OC_STATE_RESET;
      } else if(curr > button_down_time + BUTTON_APRESET_TIMEOUT) {
        oc.reset_to_ap();
      } 
      button_down_time = 0;
    }
  }
  // process led and adc
  static ulong led_toggle_timeout = 0;
  static long voltages = 0;
  if(led_blink_ms) {
    if(millis() > led_toggle_timeout) {
      // toggle led
      oc.set_led(1-oc.get_led());
      led_toggle_timeout = millis() + led_blink_ms;
      if(oc.state == OC_STATE_RFID && oc.get_led()) { //only read with WIFI off
        int volt = analogRead(PIN_ADC);
        if (volt) {
          voltages = voltages + volt;
          volt_reads++;
          voltage = ((voltages/volt_reads)/4095.f)*1.1f*(RB+RT)*10;
          DEBUG_PRINTLN(voltage);
        }      
      }
    }
  }
}

void on_ap_update() {
  server_send_html_P(ap_update_html);
}

void on_ap_upload_fin() {
  // finish update and check error
  if(!Update.end(true) || Update.hasError()) {
    server_send_result(HTML_UPLOAD_FAILED);
    return;
  }
  
  server_send_result(HTML_SUCCESS);
  //restart_ticker.once_ms(1000, oc.restart);
  if (oc.options[OPTION_SSID].sval && oc.options[OPTION_URL].sval){
    oc.options[OPTION_MOD].ival = OC_MOD_STA;
    oc.options_save();
  }
  esp_restart();
}

void on_ap_upload() {
  HTTPUpload& upload = server->upload();
  if(upload.status == UPLOAD_FILE_START){
    Serial.println(F("prepare to upload: "));
    Serial.println(upload.filename);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace()-0x1000)&0xFFFFF000;
    if(!Update.begin(maxSketchSpace)) {
      Serial.println(F("not enough space"));
    }
  } else if(upload.status == UPLOAD_FILE_WRITE) {
    Serial.print(".");
    if(Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println(F("size mismatch"));
    }
      
  } else if(upload.status == UPLOAD_FILE_END) {
    
    Serial.println(F("upload completed"));
       
  } else if(upload.status == UPLOAD_FILE_ABORTED){
    Update.end();
    Serial.println(F("upload aborted"));
  }
  delay(0);
}

void check_status_ap() {
  static ulong cs_timeout = 0;
  if(millis() > cs_timeout) {
    Serial.println(OC_FWV);
    cs_timeout = millis() + 2000;
  }
}

void do_loop() {

  static ulong connecting_timeout;
  static ulong sleep_timeout;

  switch(oc.state) {
  case OC_STATE_INITIAL:
    if(curr_mode == OC_MOD_AP) {
      led_blink_ms = LED_SLOW_BLINK;
      scanned_ssids = scan_network();
      String ap_ssid = get_ap_ssid();
      start_network_ap(ap_ssid.c_str(), NULL);
      delay(500);
      dns->setErrorReplyCode(DNSReplyCode::NoError);
      dns->start(53, "*", WiFi.softAPIP());
      server->on("/",   on_home);    
      server->on("/js", on_ap_scan);
      server->on("/cc", on_ap_change_config);
      server->on("/jt", on_ap_try_connect);
      server->on("/db", on_ap_debug);  
      server->on("/gm", on_ap_mac);    
      server->on("/update", HTTP_GET, on_ap_update);
      server->on("/update", HTTP_POST, on_ap_upload_fin, on_ap_upload);     
      server->on("/resetall",on_reset_all);
      server->onNotFound(on_home);
      server->begin();
      DEBUG_PRINTLN(F("Web Server endpoints (AP mode) registered"));
      oc.state = OC_STATE_CONNECTED;
      DEBUG_PRINTLN(WiFi.softAPIP());
      connecting_timeout = 0;
      
    } else if (oc.options[OPTION_ARD].ival == 1) {
      oc.state = OC_STATE_START_CONNECT;
    } else {
      sleep_timeout = millis() + 10000;
      oc.state = OC_STATE_RFID;
    }
    break;

  //non-Wifi connected mode with RFID reader active
  case OC_STATE_RFID:
      if(readCard()) {
        oc.state=OC_STATE_START_CONNECT;
        //log to file here?
      }
      delay(50);
      if (millis() > sleep_timeout && oc.options[OPTION_ARD].ival == 0) { do_sleep(); }
      break;

  case OC_STATE_START_CONNECT:
      sleep_timeout = millis() + 10000;
      led_blink_ms = LED_SLOW_BLINK;
      DEBUG_PRINT(F("Attempting to connect to SSID: "));
      DEBUG_PRINTLN(oc.options[OPTION_SSID].sval.c_str());
      WiFi.mode(WIFI_STA);
      start_network_sta(oc.options[OPTION_SSID].sval.c_str(), oc.options[OPTION_PASS].sval.c_str());
      //oc.config_ip();
      oc.state = OC_STATE_CONNECTING;
      connecting_timeout = millis() + 10000;
      break;

  case OC_STATE_TRY_CONNECT:
      led_blink_ms = LED_FAST_BLINK;
      DEBUG_PRINT(F("Attempting to connect to SSID: "));
      DEBUG_PRINTLN(oc.options[OPTION_SSID].sval.c_str());
      start_network_sta_with_ap(oc.options[OPTION_SSID].sval.c_str(), oc.options[OPTION_PASS].sval.c_str());
      //oc.config_ip();
      oc.state = OC_STATE_CONNECTED;
      break;
    
  case OC_STATE_CONNECTING:
    led_blink_ms = LED_SLOW_BLINK;
    if(WiFi.status() == WL_CONNECTED) {
      DEBUG_PRINT(F("Wireless connected, IP: "));
      DEBUG_PRINTLN(WiFi.localIP());
      led_blink_ms = 0;
      oc.set_led(HIGH);
      oc.state = OC_STATE_CONNECTED;
      connecting_timeout = 0;
      sleep_timeout = millis() + 10000;
    } else {
      if(millis() > connecting_timeout) {
        DEBUG_PRINTLN(F("Wifi Connecting timeout, restart"));;
        oc.play_multi_notes(3,80,260); //some low note indicating error
        if (cardKey) log_data();
        do_sleep();
      }
    }
    led_blink_ms = LED_FAST_BLINK;
    break;

  case OC_STATE_RESET:
    oc.state = OC_STATE_INITIAL;
    oc.options_reset();
    oc.log_reset();
    oc.restart();
    break;
    
  case OC_STATE_WAIT_RESTART:
    if(dns) dns->processNextRequest();  
    if(server) server->handleClient();    
    break;
    
  case OC_STATE_CONNECTED: //THIS IS THE MAIN LOOP
    if(curr_mode == OC_MOD_AP) {
      led_blink_ms = LED_SLOW_BLINK;
      dns->processNextRequest();
      server->handleClient();
      check_status_ap(); //remove this later
      connecting_timeout = 0;
      if(oc.options[OPTION_MOD].ival == OC_MOD_STA) {
        // already in STA mode, waiting to reboot
        break;
      }
      if(WiFi.status() == WL_CONNECTED && WiFi.localIP()) {
        DEBUG_PRINTLN(F("STA connected, updating option file"));
        sendData(); //newdevice data sent
        time_keeping();
        oc.options[OPTION_MOD].ival = OC_MOD_STA;
        oc.options_save();
        oc.play_multi_notes(2,50,1600);
        esp_restart();	
      }
      
    } else {
      if(WiFi.status() == WL_CONNECTED) {
        //Don't sleep if we are an admin device, for now
        if (millis() > sleep_timeout && oc.options[OPTION_ARD].ival == 0) { do_sleep(); }
        if (cardKey) { sendData(); sleep_timeout = millis()+5000; } //Have this so we don't have to turn on Wifi till we have  card
        readCard();
        delay(5);
        if (send_logs) sendLog();
        connecting_timeout = 0;
      } else {
        //oc.state = OC_STATE_INITIAL;
        if(!connecting_timeout) {
          DEBUG_PRINTLN(F("State is CONNECTED but WiFi is disconnected, start timeout counter."));
          connecting_timeout = millis()+10000;
        }
        else if(millis() > connecting_timeout) {
          DEBUG_PRINTLN(F("timeout reached, reboot"));
          do_sleep();
        }
      }
    }
    break;
  }
  //Nework independent functions, handle events like reset even when not connected
  process_ui();
}
