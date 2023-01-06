//*** debug ****************
// 0=no Debug; 
// 1=values only; 
// 2=values and info and P-buffer
// 3=values and info and T+R+P-buffer
#define DEBUG_SMA 1

#define LOOPTIME_SEC 15
#define FRIENDLY_NAME "SMA Inverter"

// SMA login password for UG_USER or UG_INSTALLER always 12 char. Unused=0x00
#define USERGROUP UG_USER
const char SmaInvPass[]= {'0', '0', '0', '0', 0, 0, 0, 0, 0, 0, 0, 0};

// SMA blutooth address -> adapt to your system
uint8_t SmaBTAddress[6]= {0x00, 0x80, 0x25, 0x2A, 0x77, 0xEE};// my SMA SMC6000TL 00:80:25:29:eb:d3 
 
// Webserver -> adapt to your system
//#define SMA_WEBSERVER

  const char *ssid        = "wifi_blokland";
  const char *password    = "blokland!1";

#define MQTT_SERVER "192.168.0.114"
#define MQTT_BASE_TOPIC "sma/solar/"
#define HOST "sma-monitor"
