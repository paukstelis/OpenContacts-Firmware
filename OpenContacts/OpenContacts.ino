/* OpenContacts Firmware
 * Based on the OpenGarage Firmware
 * Main loop wrapper for Arduino
 * Mar 2020 @ OpenContacts.io
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
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiUdp.h>
#include <time.h>
#include "OpenContacts.h"

struct tcp_pcb;
extern struct tcp_pcb* tcp_tw_pcbs;
extern "C" void tcp_abort (struct tcp_pcb* pcb);

void tcpCleanup()   // losing bytes work around
{  while(tcp_tw_pcbs)
  {    tcp_abort(tcp_tw_pcbs);  }}
void do_setup();
void do_loop();

void setup() {
    do_setup();
}

void loop() {
  do_loop();
  tcpCleanup();       
}
