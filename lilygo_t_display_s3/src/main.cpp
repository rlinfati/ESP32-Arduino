#include <esp_wifi.h>
#include <WiFiProv.h>
#include <HTTPClient.h>
#include "TFT_eSPI.h"
#include "libpax_api.h"
#include "OneButton.h"

TFT_eSPI tft = TFT_eSPI();

struct count_payload_t count_from_libpax;
unsigned long blecount = 0;
unsigned long wificount = 0;
void libpax_counter_reset();

OneButton button(BUTTON_RST);

unsigned long tstart = 0;
unsigned long treset = 0;
unsigned long timeoutms = 60 * 1e3;

String ippub = "0.0.0.0";

void process_count(void) {
  blecount = count_from_libpax.ble_count;
  wificount = count_from_libpax.wifi_count;
  return;
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Setup...");

  tft.init();
  tft.setTextSize(1); // 28 x 40
  tft.setTextSize(2); // 14 x 20
  tft.setRotation(1); // 10 x 28
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_LIGHTGREY);

  button.attachClick([]() {
    Serial.println("Restarting...");
    tft.fillScreen(TFT_GREEN);
    tft.setCursor(0, 0);
    tft.println("Restarting...");
    esp_restart();
  });

  button.attachDoubleClick([]() {
    Serial.println("Erasing Provision...");
    tft.fillScreen(TFT_RED);
    tft.setCursor(0, 0);
    tft.println("Erasing Provision...");

    delay(timeoutms);

    wifi_config_t conf;
    conf.sta.ssid[0] = 0;
    esp_wifi_set_config((wifi_interface_t)ESP_IF_WIFI_STA, &conf);

    esp_restart();
  });

  WiFiProv.beginProvision();

  Serial.println("WiFi...");
  tft.fillScreen(TFT_BLACK);
  tft.setCursor(0, 0);
  tft.printf("WiFi...");

  tstart = millis();
  while ( millis()-tstart < timeoutms) {
    Serial.printf(".");
    tft.printf(".");
    delay(1e3);
    if ( WiFi.isConnected() ) break;
  }

  configTzTime("<-04>4<-03>,M9.1.6/24,M4.1.6/24", "time.cloudflare.com", "ntp.ubiobio.cl");

  Serial.println("SNTP...");
  tft.fillScreen(TFT_DARKGREY);
  tft.setCursor(0, 0);
  tft.printf("SNTP...");

  tstart = millis();
  while ( millis()-tstart < timeoutms) {
    Serial.printf(".");
    tft.printf(".");
    struct tm timeinfo;
    if ( getLocalTime(&timeinfo) ) break;
  }

  if ( !WiFi.isConnected() ) {
    Serial.println("WiFi is Not Connected...");
    tft.fillScreen(TFT_RED);
    tft.setCursor(0, 0);
    tft.println("WiFi is Not Connected...");

    delay(timeoutms);

    esp_restart();
  }

  struct libpax_config_t configpax;
  libpax_default_config(&configpax);
  configpax.wifi_channel_map = WIFI_CHANNEL_ALL;
  configpax.wificounter = 1;
  configpax.blecounter = 1;
  libpax_update_config(&configpax);
  libpax_counter_init(process_count, &count_from_libpax, 1, 1);
  libpax_counter_start();

  HTTPClient http;
  http.begin("http://ifconfig.me/ip");
  if ( http.GET() == HTTP_CODE_OK ) ippub = http.getString();

  tstart = millis();
  treset = millis();

  return;
}

void loop() {
  button.tick();

  if ( millis()-tstart < 1e3 ) {
    return;
  }

  struct tm timeinfo;
  getLocalTime(&timeinfo);

  Serial.printf("* Lilygo Display S3\n");
  Serial.printf("* Date/time %04i-%02i-%02i %02i:%02i:%02i\n",
    1900+timeinfo.tm_year, 1+timeinfo.tm_mon, timeinfo.tm_mday,
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  Serial.printf("* SSID %s %s\n", WiFi.SSID().c_str(), WiFi.BSSIDstr().c_str());
  Serial.printf("* IP %s %s\n", WiFi.localIP().toString().c_str(), ippub.c_str());
  Serial.printf("* BLE: %i\n", blecount);
  Serial.printf("* WiFi: %i\n", wificount);
  Serial.printf("\n");

  tft.fillScreen(TFT_DARKGREY);
  tft.setCursor(0, 0);
  tft.printf(" Lilygo Display S3\n");
  tft.printf(" Date: %04i-%02i-%02i\n",
    1900+timeinfo.tm_year, 1+timeinfo.tm_mon, timeinfo.tm_mday);
  tft.printf(" Time: %02i:%02i:%02i\n",
    timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
  tft.printf("\n");
  tft.printf(" SSID: %s\n", WiFi.SSID().c_str());
  tft.printf(" BSSID: %s\n", WiFi.BSSIDstr().c_str());
  tft.printf(" localIP: %s\n", WiFi.localIP().toString().c_str());
  tft.printf(" publicIP: %s\n", ippub.c_str());
  tft.printf(" BLE: %i\n", blecount);
  tft.printf(" WiFi: %i\n", wificount);

  if ( millis()-treset > 5 * timeoutms ) {
    HTTPClient httpIP;
    httpIP.begin("http://ifconfig.me/ip");
    if ( httpIP.GET() == HTTP_CODE_OK ) {
      ippub = httpIP.getString();
      Serial.println("* WiFiClient  OK");
    } else {
      ippub = "0.0.0.0";
      Serial.println("* WiFiClient NOK");
      tft.fillScreen(TFT_RED);
    }

    HTTPClient http;
    String url = "https://script.google.com/macros/s/AKfycbxq3BQBh3QopTAkpj93Tb5ycSd96spx2wE_1zAsz8whP72kJWmy-TdeyEEU4bQgY27f/exec";
    String postdata = String("device=lilygo_t_display_s3")
      + "&ssid=" + WiFi.SSID()
      + "&bssid=" + WiFi.BSSIDstr()
      + "&ipprv=" + WiFi.localIP().toString()
      + "&ippub=" + ippub
      + "&wifi=" + wificount
      + "&ble=" + blecount;
    http.begin(url);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");
    http.POST(postdata);

    libpax_counter_reset();
    treset = millis();
  }

  tstart = millis();

  return;
}
