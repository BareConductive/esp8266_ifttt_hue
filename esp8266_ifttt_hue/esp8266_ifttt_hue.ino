/*******************************************************************************

 Bare Conductive wireless Philips Hue lightswitch
 ------------------------------------------------

 esp8366_ifttt_hue.ino - touch triggered WiFi lightswitch

 Bare Conductive code written by Carlo Palumbo, Pascal Loose

 This work is licensed under a MIT license https://opensource.org/licenses/MIT

 Copyright (c) 2021, Bare Conductive

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.

*******************************************************************************/

// compiler error handling
#include "Compiler_Errors.h"

// touch includes
#include <MPR121.h>

// wifi includes
#include "WiFiEsp.h"
#include "AnotherIFTTTWebhook.h"

// touch constants
const uint32_t BAUD_RATE = 115200;
const uint8_t MPR121_ADDR = 0x5C;
const uint8_t MPR121_INT = 4;

// wifi constants
char ssid[] = "";  // change to your network SSID (name)
char pass[] = "";  // change to your network password
int status = WL_IDLE_STATUS;  // the Wifi radio's status
#define IFTTT_Key ""  // change to your IFTTT webhook key
#define IFTTT_Event ""  // change to your IFTTT event name

void setup()
{
  // initialize serial for debugging
  Serial.begin(BAUD_RATE);
  // initialize serial for ESP module
  Serial1.begin(BAUD_RATE);
  // initialize ESP module
  WiFi.init(&Serial1);

  if (!MPR121.begin(MPR121_ADDR)) {
    Serial.println("error setting up MPR121");
    switch (MPR121.getError()) {
      case NO_ERROR:
        Serial.println("no error");
        break;
      case ADDRESS_UNKNOWN:
        Serial.println("incorrect address");
        break;
      case READBACK_FAIL:
        Serial.println("readback failure");
        break;
      case OVERCURRENT_FLAG:
        Serial.println("overcurrent on REXT pin");
        break;
      case OUT_OF_RANGE:
        Serial.println("electrode out of range");
        break;
      case NOT_INITED:
        Serial.println("not initialised");
        break;
      default:
        Serial.println("unknown error");
        break;
    }
    while (1);
  }

  MPR121.setInterruptPin(MPR121_INT);

  MPR121.setTouchThreshold(40);
  MPR121.setReleaseThreshold(20);

  // check for the presence of the module
  if (WiFi.status() == WL_NO_SHIELD) {
    Serial.println("WiFi module not present");
    while (true);
  }

  // attempt to connect to WiFi network
  while ( status != WL_CONNECTED) {
    Serial.print("Attempting to connect to WPA SSID: ");
    Serial.println(ssid);
    status = WiFi.begin(ssid, pass); // connect to WPA/WPA2 network
  }

  Serial.println("You're connected to the network");
  printWifiStatus();
  Serial.println();
  Serial.println("Starting connection to server...");
}

void loop() {
  MPR121.updateAll();

  if (MPR121.getNumTouches() <= 1) {
    if (MPR121.isNewTouch(11)) {
      Serial.println("Sending the event to IFTTT");
      send_webhook(IFTTT_Event, IFTTT_Key);
    }
  }
}

void send_webhook(char *MakerIFTTT_Event, char *MakerIFTTT_Key) {
  client.connect("maker.ifttt.com", 80);  // connect to the Maker event server

  // construct the POST request
  char post_rqst[256];    // hand-calculated to be big enough

  char *p = post_rqst;
  p = append_str(p, "POST /trigger/");
  p = append_str(p, MakerIFTTT_Event);
  p = append_str(p, "/with/key/");
  p = append_str(p, MakerIFTTT_Key);
  p = append_str(p, " HTTP/1.1\r\n");
  p = append_str(p, "Host: maker.ifttt.com\r\n");
  p = append_str(p, "Content-Type: application/json\r\n");
  p = append_str(p, "Content-Length: ");

  char *content_length_here = p;  // remember where the content length will go

  p = append_str(p, "NN\r\n");  // it's always two digits, so reserve space for them (the NN)
  p = append_str(p, "\r\n");  // end of headers

  char *json_start = p;  // construct the JSON; remember where we started so we will know len

  // go back and fill in the JSON length
  // we just know this is at most 2 digits (and need to fill in both)
  int i = strlen(json_start);
  content_length_here[0] = '0' + (i / 10);
  content_length_here[1] = '0' + (i % 10);

  // finally we are ready to send the POST to the server!
  client.print(post_rqst);
  client.stop();
}

void printWifiStatus() {
  // print the SSID of the attached network
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());

  // print the WiFi module's IP address
  IPAddress ip = WiFi.localIP();
  Serial.print("IP Address: ");
  Serial.println(ip);

  // print the received signal strength
  long rssi = WiFi.RSSI();
  Serial.print("Signal strength (RSSI):");
  Serial.print(rssi);
  Serial.println(" dBm");
}
