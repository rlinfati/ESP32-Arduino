#include <esp_wifi.h>
#include <WiFiProv.h>
#include <ESPmDNS.h>
#include <NetBIOS.h>
#include <HTTPClient.h>
#include <WebServer.h>
#include "TFT_eSPI.h"
#include "libpax_api.h"
#include "OneButton.h"
#include "FastLED.h"

TFT_eSPI tft = TFT_eSPI();
struct count_payload_t count_from_libpax;
unsigned long blecount = 0;
unsigned long wificount = 0;
OneButton button(BUTTON_RST);
CRGB leds;
unsigned long start = 0;
unsigned long timeoutms = 60 * 1e3;
String ippub = "0.0.0.0";
WebServer HTTPServer(80);

void process_count(void) {
  blecount = count_from_libpax.ble_count;
  wificount = count_from_libpax.wifi_count;
  return;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  tft.init();
  tft.setTextSize(1);
  tft.setRotation(3);
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY);

  button.attachClick([]() {
    tft.fillScreen(TFT_GREEN);
    esp_restart();
  });

  button.attachDoubleClick([]() {
    tft.fillScreen(TFT_RED);
    wifi_config_t conf;
    conf.sta.ssid[0] = 0;
    esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);
    esp_restart();
  });

  WiFiProv.beginProvision();

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  start = millis();
  tft.printf("WiFi");
  while ( millis()-start < timeoutms) {
    tft.printf(".");
    delay(1000);
    if ( WiFi.isConnected() ) break;
  }

  WiFi.enableIpV6();
  configTzTime("<-04>4<-03>,M9.1.6/24,M4.1.6/24", "time.cloudflare.com", "ntp.ubiobio.cl");

  tft.fillScreen(TFT_DARKGREY);
  tft.setCursor(0, 0);
  start = millis();
  tft.printf("SNTP");
  while ( millis()-start < timeoutms) {
    tft.printf(".");
    struct tm timeinfo;
    if ( getLocalTime(&timeinfo) ) break;
  }

  if ( !WiFi.isConnected() ) {
    tft.fillScreen(TFT_RED);
    tft.setCursor(0, 0);
    tft.printf("Erasing Provision...");
    wifi_config_t conf;
    conf.sta.ssid[0] = 0;
    esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);
    delay(60 * 1e3);
    esp_restart();
  }

  struct libpax_config_t configpax;
  libpax_default_config(&configpax);
  configpax.wifi_channel_map = WIFI_CHANNEL_ALL;
  configpax.wificounter = 0;
  configpax.blecounter = 1;
  libpax_update_config(&configpax);
  libpax_counter_init(process_count, &count_from_libpax, 60, 0); 
  libpax_counter_start();

  MDNS.begin("LilygoESPS3");
  NBNS.begin("LilygoESPS3");

  HTTPClient http;
  http.begin("http://ifconfig.me/ip");
  if ( http.GET() == HTTP_CODE_OK ) ippub = http.getString();

  HTTPServer.onNotFound([]() {
    HTTPServer.send(200, "text/plain", "Hello LilygoESPS3\n");
  });
  HTTPServer.begin();

  FastLED.addLeds<APA102, xDATA_PIN, xCLOCK_PIN, BGR>(&leds, 1);
  FastLED.clear(true);
  FastLED.showColor(CRGB::Blue);

  return;
}

void loop() {
  button.tick();
  HTTPServer.handleClient();

  if ( millis()-start < 1e3 ) {
    return;
  }
  start = millis();
  
  struct tm timeinfo;
  getLocalTime(&timeinfo);

  tft.fillScreen(TFT_DARKGREY);
  tft.setCursor(0, 0);
  tft.printf("\n");
  tft.printf(" %04i-%02i-%02i  ",
    1900+timeinfo.tm_year, 1+timeinfo.tm_mon, timeinfo.tm_mday);
  tft.printf(" %02i:%02i:%02i\n", 
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  tft.printf("\n");
  tft.printf("  SSID: %s\n", WiFi.SSID().c_str());
  tft.printf(" BSSID: %s\n", WiFi.BSSIDstr().c_str());
  tft.printf("\n");
  tft.printf(" IPprv: %s\n", WiFi.localIP().toString().c_str());
  tft.printf(" IPpub: %s\n", ippub.c_str());
  tft.printf("\n");
  tft.printf(" # BLE: %i\n", blecount);
  
  return;
}
