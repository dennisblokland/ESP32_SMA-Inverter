/* MIT License

Copyright (c) 2022 Lupo135

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
*/

#include <esp_wifi.h> 
#include <WebServer.h>

WebServer server(80);
#define HTTP_BUF_MAX 2048
char httpBuf[HTTP_BUF_MAX];

bool setupWebserver() {
  Serial.printf("Connecting to %s ", ssid);
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  int i=0;
  for (i=0; i<50; i++) {
    if (WiFi.status() == WL_CONNECTED) 
      break;
    delay(500);
    Serial.print(".");
  }
  
  if (i>=100) { // without connection go to sleep 
    Serial.println("No Connection!!");
    WiFi.disconnect(true);
    delay(1000);
    return false;
  }
  Serial.print(" Connected to IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

void SmaServeTable() {
int length = 0;
uint32_t reloadSec;
if (btConnected) reloadSec = (((nextTime-millis())/1000)+1); // wait until new values
else             reloadSec = 60; 
if (reloadSec>120) reloadSec = 10;

length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
      "<html><head><meta http-equiv=\"refresh\" content=\"%d\"><body>\n\
<style type=\"text/css\">\n\
#dT {align:middle; border-collapse:collapse;}\n\
#dT td, #dT th {align:middle; text-align:center; font-size:30%; border:1px solid #98bf21; padding:3px 7px 2px 7px; }\n\
#dT tr:nth-child(odd) {background:#EAF2D3;} .vCenter {display:flex;justify-content:center;margin:5px;}\n\
progress {text-align: center;} progress:after{content: attr(value);} </style>\n\
<progress value=\"%d\" max=\"%d\" id=\"pB\"></progress><br>\n\
<table align=\"left\" id=\"dT\" vspace=5 style=\"margin-bottom:5px;\">\n", reloadSec, reloadSec, reloadSec);

if (btConnected) {
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
"<tr><td>Pac</td><td>%1.3f kW</td></tr>\n\
 <tr><td>Udc</td><td>%1.1f V</td></tr>\n\
 <tr><td>Idc</td><td>%1.3f A</td></tr>\n\
 <tr><td>Uac</td><td>%1.1f V</td></tr>\n\
 <tr><td>Iac</td><td>%1.3f A</td></tr>\n"
 , tokW(pInvData->Pac)
 , toVolt(pInvData->Udc)
 , toAmp(pInvData->Idc)
 , toVolt(pInvData->Uac)
 , toAmp(pInvData->Iac));
} else {
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
        "<tr><td>Bluetooth</td><td>offline</td></tr>\n");
}
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
"<tr><td>E-Today</td><td>%1.3f kWh</td></tr>\n\
 <tr><td>E-Total</td><td>%1.3f kWh</td></tr>\n"
 , tokWh(pInvData->EToday)
 , tokWh(pInvData->ETotal));

if (btConnected)
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
"<tr><td>Frequency</td><td>%1.2f Hz</td></tr>\n\
 <tr><td>Efficiency</td><td> %1.2f %%</td></tr>\n"
  , toHz(pInvData->Freq)
  , toPercent(pInvData->Eta));
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length, "<tr><td>Last-GMT</td><td>");
  length += printUnixTime(httpBuf+length, pInvData->LastTime);
  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length, "</td></tr>\n");

  length += snprintf(httpBuf+length, HTTP_BUF_MAX-length,
"<script>var tm=%d;var dTimer=setInterval(function(){\n\
if(tm<=0){clearInterval(dTimer);}\n\
document.getElementById(\"pB\").value=tm;\n\ 
tm -= 1;},1000); </script></body></html>", reloadSec);

  server.send(200, "text/html", httpBuf);
}

