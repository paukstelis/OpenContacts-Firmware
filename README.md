# OpenContacts Firmware
* OpenContacts is a WiFi-connected RFID logger to enable contact tracing and real-time occupancy monitoring at the room level.
* This firmware runs on the ESP32 MCU and uses an off-the-shelf RC522 RFID peripheral for NFC/RFID scanning (MiFare 13.56MHz).
* OpenContacts was adapted from the OpenGarage firmware.
# Basic Operation
* Devices begin in Access Point mode where backend server URL, building, room, and wireless network information are setup. Device enters station mode and is ready for operation.
* Admin mode allows the device to work strictly as a WiFi connected card reader to return RFID UIDs via the backend server.
* Device should be installed at a door threshold and contains a PIR sensor to take the unit out of deep sleep when someone crosses the threshold. 
* A buzzer and LED alert users that enter or exit the room to swipe their RFID tag.
* If a WiFi connection cannot be established or the server backend is not reachable, swipes are logged locally until the next server connection.
* Device runs on 2XAA batteries.


