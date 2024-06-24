#include <esp_wifi.h>
#include <WiFiProv.h>
#include <HTTPClient.h>
#include "TFT_eSPI.h"
#include "libpax_api.h"
#include "OneButton.h"
#include "FastLED.h"
#include "USB.h"
#include "USBMSC.h"

TFT_eSPI tft = TFT_eSPI();

struct count_payload_t count_from_libpax;
unsigned long blecount = 0;
unsigned long wificount = 0;
void libpax_counter_reset();

OneButton button(BUTTON_RST);

CRGB leds;

unsigned long tstart = 0;
unsigned long treset = 0;
unsigned long timeoutms = 60 * 1e3;

String ippub = "0.0.0.0";

void process_count(void) {
  blecount = count_from_libpax.ble_count;
  wificount = count_from_libpax.wifi_count;
  return;
}

USBMSC MSC;
#define FAT_U8(v)          ((v) & 0xFF)
#define FAT_U16(v)         FAT_U8(v), FAT_U8((v) >> 8)
#define FAT_U32(v)         FAT_U8(v), FAT_U8((v) >> 8), FAT_U8((v) >> 16), FAT_U8((v) >> 24)
#define FAT_MS2B(s, ms)    FAT_U8(((((s) & 0x1) * 1000) + (ms)) / 10)
#define FAT_HMS2B(h, m, s) FAT_U8(((s) >> 1) | (((m) & 0x7) << 5)), FAT_U8((((m) >> 3) & 0x7) | ((h) << 3))
#define FAT_YMD2B(y, m, d) FAT_U8(((d) & 0x1F) | (((m) & 0x7) << 5)), FAT_U8((((m) >> 3) & 0x1) | ((((y) - 1980) & 0x7F) << 1))
#define FAT_TBL2B(l, h)    FAT_U8(l), FAT_U8(((l >> 8) & 0xF) | ((h << 4) & 0xF0)), FAT_U8(h >> 4)
#define README_CONTENTS "rodrigo@linfati.cl\r\n"
static const uint32_t DISK_SECTOR_COUNT = 2 * 8;   // 8KB is the smallest size that windows allow to mount
static const uint16_t DISK_SECTOR_SIZE = 512;      // Should be 512
static const uint16_t DISC_SECTORS_PER_TABLE = 1;  //each table sector can fit 170KB (340 sectors)

static uint8_t msc_disk[DISK_SECTOR_COUNT][DISK_SECTOR_SIZE] = {
  //------------- Block0: Boot Sector -------------//
  {                                                        // Header (62 bytes)
   0xEB, 0x3C, 0x90,                                       //jump_instruction
   'M', 'S', 'D', 'O', 'S', '5', '.', '0',                 //oem_name
   FAT_U16(DISK_SECTOR_SIZE),                              //bytes_per_sector
   FAT_U8(1),                                              //sectors_per_cluster
   FAT_U16(1),                                             //reserved_sectors_count
   FAT_U8(1),                                              //file_alloc_tables_num
   FAT_U16(16),                                            //max_root_dir_entries
   FAT_U16(DISK_SECTOR_COUNT),                             //fat12_sector_num
   0xF8,                                                   //media_descriptor
   FAT_U16(DISC_SECTORS_PER_TABLE),                        //sectors_per_alloc_table;//FAT12 and FAT16
   FAT_U16(1),                                             //sectors_per_track;//A value of 0 may indicate LBA-only access
   FAT_U16(1),                                             //num_heads
   FAT_U32(0),                                             //hidden_sectors_count
   FAT_U32(0),                                             //total_sectors_32
   0x00,                                                   //physical_drive_number;0x00 for (first) removable media, 0x80 for (first) fixed disk
   0x00,                                                   //reserved
   0x29,                                                   //extended_boot_signature;//should be 0x29
   FAT_U32(0x1234),                                        //serial_number: 0x1234 => 1234
   'T', 'i', 'n', 'y', 'U', 'S', 'B', ' ', 'M', 'S', 'C',  //volume_label padded with spaces (0x20)
   'F', 'A', 'T', '1', '2', ' ', ' ', ' ',                 //file_system_type padded with spaces (0x20)

   // Zero up to 2 last bytes of FAT magic code (448 bytes)
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,

   //boot signature (2 bytes)
   0x55, 0xAA
  },

  //------------- Block1: FAT12 Table -------------//
  {
    FAT_TBL2B(0xFF8, 0xFFF), FAT_TBL2B(0xFFF, 0x000)  // first 2 entries must be 0xFF8 0xFFF, third entry is cluster end of readme file
  },

  //------------- Block2: Root Directory -------------//
  {
    // first entry is volume label
    'E', 'S', 'P', '3', '2', 'S', '3', ' ', 'M', 'S', 'C',
    0x08,                                                                                                                 //FILE_ATTR_VOLUME_LABEL
    0x00, FAT_MS2B(0, 0), FAT_HMS2B(0, 0, 0), FAT_YMD2B(0, 0, 0), FAT_YMD2B(0, 0, 0), FAT_U16(0), FAT_HMS2B(13, 42, 30),  //last_modified_hms
    FAT_YMD2B(2018, 11, 5),                                                                                               //last_modified_ymd
    FAT_U16(0), FAT_U32(0),

    // second entry is readme file
    'R', 'E', 'A', 'D', 'M', 'E', ' ', ' ',  //file_name[8]; padded with spaces (0x20)
    'T', 'X', 'T',                           //file_extension[3]; padded with spaces (0x20)
    0x20,                                    //file attributes: FILE_ATTR_ARCHIVE
    0x00,                                    //ignore
    FAT_MS2B(1, 980),                        //creation_time_10_ms (max 199x10 = 1s 990ms)
    FAT_HMS2B(13, 42, 36),                   //create_time_hms [5:6:5] => h:m:(s/2)
    FAT_YMD2B(2018, 11, 5),                  //create_time_ymd [7:4:5] => (y+1980):m:d
    FAT_YMD2B(2020, 11, 5),                  //last_access_ymd
    FAT_U16(0),                              //extended_attributes
    FAT_HMS2B(13, 44, 16),                   //last_modified_hms
    FAT_YMD2B(2019, 11, 5),                  //last_modified_ymd
    FAT_U16(2),                              //start of file in cluster
    FAT_U32(sizeof(README_CONTENTS) - 1)     //file size
  },

  //------------- Block3: Readme Content -------------//
  README_CONTENTS
};

static int32_t onWrite(uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
  memcpy(msc_disk[lba] + offset, buffer, bufsize);
  return bufsize;
}

static int32_t onRead(uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
  memcpy(buffer, msc_disk[lba] + offset, bufsize);
  return bufsize;
}

static bool onStartStop(uint8_t power_condition, bool start, bool load_eject) {
  return true;
}

static void usbEventCallback(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
}

void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
  Serial.println("Setup...");

  tft.init();
  tft.setTextSize(1); // 13 x 20
  tft.setRotation(1); // 10 x 26
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

  FastLED.addLeds<APA102, xDATA_PIN, xCLOCK_PIN, BGR>(&leds, 1);
  FastLED.clear(true);
  FastLED.showColor(CRGB::Blue);

  USB.onEvent(usbEventCallback);
  MSC.vendorID("LILYGO");
  MSC.productID("T-Dongle-S3");
  MSC.productRevision("1.0");
  MSC.onStartStop(onStartStop);
  MSC.onRead(onRead);
  MSC.onWrite(onWrite);
  MSC.mediaPresent(true);
  MSC.begin(DISK_SECTOR_COUNT, DISK_SECTOR_SIZE);
  USB.begin();

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

  Serial.printf("* Lilygo Dongle S3\n");
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
  tft.printf(" Lilygo Dongle S3\n");
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
      FastLED.showColor(CRGB::Green);
    } else {
      ippub = "0.0.0.0";
      Serial.println("* WiFiClient NOK");
      FastLED.showColor(CRGB::Red);
      tft.fillScreen(TFT_RED);
    }

    HTTPClient http;
    String url = "https://script.google.com/macros/s/AKfycbxq3BQBh3QopTAkpj93Tb5ycSd96spx2wE_1zAsz8whP72kJWmy-TdeyEEU4bQgY27f/exec";
    String postdata = String("device=lilygo_t_dongle_s3")
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
