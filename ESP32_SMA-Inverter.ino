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

#include <Esp.h>
#include "Arduino.h"
#include "BluetoothSerial.h"
#include "EspMQTTClient.h"

#include "Config.h"
#include "Utils.h"

const uint16_t AppSUSyID = 125;
uint32_t AppSerial;
//
#define maxpcktBufsize 256
uint8_t BTrdBuf[maxpcktBufsize]; //  Serial.printf("Connecting to %s\n", ssid);
uint8_t pcktBuf[maxpcktBufsize];
uint16_t pcktBufPos = 0;
uint16_t pcktBufMax = 0; // max. used size of PcktBuf
uint16_t pcktID = 1;
const char BTPin[] = {'0', '0', '0', '0', 0}; // BT pin Always 0000. (not login passcode!)

uint8_t EspBTAddress[6]; // is retrieved from BT packet
uint32_t nextTime = 0;
uint32_t nextConnect = 0;
uint16_t cycle = 0;
uint8_t errCnt = 0;
bool btConnected = false;
EspMQTTClient client(
    ssid,
    password,
    MQTT_SERVER,
    "", // Can be omitted if not needed
    "", // Can be omitted if not needed
    HOST);
#include "SMA_bluetooth.h"
#include "SMA_Inverter.h"

#ifdef SMA_WEBSERVER
#include "SMA_webserver.h"
#endif

void setup()
{
  Serial.begin(115200);
  delay(1000);
  pInvData->SUSyID = 0x7d;
  pInvData->Serial = 0;

  // reverse inverter BT address
  for (uint8_t i = 0; i < 6; i++)
    pInvData->BTAddress[i] = SmaBTAddress[5 - i];
  DEBUG2_PRINTF("pInvData->BTAddress: %02X:%02X:%02X:%02X:%02X:%02X\n",
                pInvData->BTAddress[5], pInvData->BTAddress[4], pInvData->BTAddress[3],
                pInvData->BTAddress[2], pInvData->BTAddress[1], pInvData->BTAddress[0]);
  // *** Start BT
  SerialBT.begin("ESP32test", true); // "true" creates this device as a BT Master.
  SerialBT.setPin(&BTPin[0]);

  client.setMaxPacketSize(512); // must be big enough to send home assistant config
  client.enableMQTTPersistence();
  client.enableDebuggingMessages();       // Enable debugging messages sent to serial output
  client.enableLastWillMessage(MQTT_BASE_TOPIC "LWT", "offline", true);

#ifdef SMA_WEBSERVER
  // *** Start WIFI and WebServer
  setupWebserver();
  server.on("/", SmaServeTable);
  server.begin();
  DEBUG1_PRINT("WebServer started");
#endif

  // if ((getInverterData(OperationTime)) != 0)     DEBUG1_PRINTLN("OperationTime error!"    ); // OperTime + OperTime
}

// **** Loop ************
void loop()
{
  client.loop();
  if ((nextTime < millis()) && (cycle > 4))
  {
    nextTime += LOOPTIME_SEC * 1000;
    if (btConnected)
      cycle = 0; // -> start reading inverter
    if (client.isConnected())
    {
      client.publish(MQTT_BASE_TOPIC "wifi_signal", String(WiFi.RSSI()));
    }
    if (nextTime < millis())
      nextTime = LOOPTIME_SEC * 1000 + millis();
  }

  // connect or reconnect after connection lost
  if ((nextConnect < millis()) && (!btConnected))
  {
    nextConnect = millis() + 10 * 60 * 1000; // 10 minutes
    pcktID = 1;
    // **** Connect SMA **********
    DEBUG1_PRINT("\nConnecting SMA inverter: ");
    if (SerialBT.connect(SmaBTAddress))
    {
      btConnected = true;
      cycle = 0;

      // **** Initialize SMA *******
      DEBUG1_PRINTLN("connected");
      E_RC rc = initialiseSMAConnection();

      getBT_SignalStrength();

      // **** logon SMA ************
      DEBUG1_PRINT("\n*** logonSMAInverter");
      rc = logonSMAInverter(SmaInvPass, USERGROUP);
    }
  }

  if (btConnected)
  {
    switch (cycle)
    {
    case 0:
      if ((getInverterData(SpotACTotalPower)) != 0)
      {
        DEBUG1_PRINTLN("SpotACTotalPower error!"); // Pac
        errCnt++;
      }
      cycle++;
      break;
    case 1:
      if ((getInverterData(SpotDCVoltage)) != 0)
      {
        DEBUG1_PRINTLN("getSpotDCVoltage error!"); // Udc + Idc
        errCnt++;
      }
      cycle++;
      break;
    case 2:
      if ((getInverterData(SpotACVoltage)) != 0)
      {
        DEBUG1_PRINTLN("getSpotACVoltage error!"); // Uac + Iac
        errCnt++;
      }
      cycle++;
      break;
    case 3:
      if ((getInverterData(EnergyProduction)) != 0)
      {
        DEBUG1_PRINTLN("EnergyProduction error!"); // E-Total + E-Today
        errCnt++;
      }
      cycle++;
      break;
    case 4:
      if ((getInverterData(SpotGridFrequency)) != 0)
      {
        DEBUG1_PRINTLN("SpotGridFrequency error!");
        errCnt++;
      }

      DEBUG1_PRINT("\n *************************");
      if (errCnt > 2)
      {
        btConnected = false;
        errCnt = 0;
      }
      cycle++;
      break;
      // case 5: if ((getInverterData(SpotDCPower)) != 0)   DEBUG1_PRINTLN("getSpotDCPower error!"); //pcktBuf[23]=15 error!
      // case 6: if ((getInverterData(SpotACPower)) != 0)   DEBUG1_PRINTLN("SpotACPower error!"   ); //pcktBuf[23]=15 error!
      // case 7: if ((getInverterData(InverterTemp)) != 0)  DEBUG1_PRINTLN("InverterTemp error!"  ); //pcktBuf[23]=15 error!
      // case 8: if ((getInverterData(OperationTime)) != 0) DEBUG1_PRINTLN("OperationTime error!" ); // OperTime + OperTime
    default:
      if (btConnected)
        delay(500);
      cycle++;
      break;
    } // switch
  }   // connected

#ifdef SMA_WEBSERVER
  server.handleClient();
#endif
}
void onConnectionEstablished()
{
  client.publish(MQTT_BASE_TOPIC "LWT", "online", true);

  client.publish("homeassistant/sensor/" HOST "/signal/config", "{\"name\": \"" FRIENDLY_NAME " Signal Strength\", \"state_topic\": \"" MQTT_BASE_TOPIC "wifi_signal\", \"unique_id\": \"" HOST "-signal\", \"unit_of_measurement\": \"dB\", \"device\": {\"identifiers\": [\"" HOST "-device\"], \"name\": \"" FRIENDLY_NAME "\"}}", true);
  client.publish("homeassistant/sensor/" HOST "/generation_today/config", "{\"name\": \"" FRIENDLY_NAME " Power Generation Today\", \"device_class\": \"energy\", \"state_topic\": \"" MQTT_BASE_TOPIC "generation_today\", \"unique_id\": \"" HOST "-generation_today\", \"unit_of_measurement\": \"Wh\", \"state_class\": \"total_increasing\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/generation_total/config", "{\"name\": \"" FRIENDLY_NAME " Power Generation Total\", \"device_class\": \"energy\", \"state_topic\": \"" MQTT_BASE_TOPIC "generation_total\", \"unique_id\": \"" HOST "-generation_total\", \"unit_of_measurement\": \"Wh\", \"state_class\": \"total_increasing\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/instant_ac/config", "{\"name\": \"" FRIENDLY_NAME " Instantinous AC Power\", \"device_class\": \"energy\", \"state_topic\": \"" MQTT_BASE_TOPIC "instant_ac\", \"unique_id\": \"" HOST "-instant_ac\", \"unit_of_measurement\": \"W\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
 
  client.publish("homeassistant/sensor/" HOST "/ac_voltage/config", "{\"name\": \"" FRIENDLY_NAME " Instantinous AC Voltage\",  \"state_topic\": \"" MQTT_BASE_TOPIC "ac_voltage\", \"unique_id\": \"" HOST "-ac_voltage\", \"unit_of_measurement\": \"V\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/ac_current/config", "{\"name\": \"" FRIENDLY_NAME " Instantinous AC Current\",  \"state_topic\": \"" MQTT_BASE_TOPIC "ac_current\", \"unique_id\": \"" HOST "-ac_current\", \"unit_of_measurement\": \"A\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/frequency/config", "{\"name\": \"" FRIENDLY_NAME " Instantinous AC Frequency\",  \"state_topic\": \"" MQTT_BASE_TOPIC "frequency\", \"unique_id\": \"" HOST "-frequency\", \"unit_of_measurement\": \"Hz\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/dc_voltage/config", "{\"name\": \"" FRIENDLY_NAME " Instantinous DC Voltage\",  \"state_topic\": \"" MQTT_BASE_TOPIC "dc_voltage\", \"unique_id\": \"" HOST "-dc_voltage\", \"unit_of_measurement\": \"V\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);
  client.publish("homeassistant/sensor/" HOST "/efficiency/config", "{\"name\": \"" FRIENDLY_NAME " Inverter Efficiency\",  \"state_topic\": \"" MQTT_BASE_TOPIC "efficiency\", \"unique_id\": \"" HOST "-efficiency\", \"unit_of_measurement\": \"%\", \"state_class\": \"measurement\", \"device\": {\"identifiers\": [\"" HOST "-device\"]} }", true);

  }