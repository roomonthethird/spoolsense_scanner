#include <WiFi.h>
#include <time.h>

#include "ConfigurationManager.h"
#include "BluetoothManager.h"
#include "ApplicationManager.h"
#include "NFCManager.h"
#include "SpoolmanManager.h"
#include "HomeAssistantManager.h"
#include "LCDManager.h"
#include "WebServerManager.h"
#include "BoardPins.h"

#ifdef ENABLE_LCD
#include <Wire.h>
#endif

#ifdef ENABLE_STATUS_LED
#include "LEDManager.h"
LEDManager ledManager;
#endif

// Global HTTP mutex for serializing WiFi HTTP requests
SemaphoreHandle_t g_httpMutex = nullptr;

#ifdef ENABLE_LCD
// LCD I2C pins from BoardPins.h
LCDManager lcdManager(0x27, 16, 2);
#endif

void initWiFi() {
  auto& config = ConfigurationManager::getInstance();

  if (strlen(config.getWiFiSSID()) == 0) {
    Serial.println("WiFi SSID not configured - skipping WiFi");
#ifdef ENABLE_LCD
    lcdManager.updateScreen("WiFi: no SSID", "Check UserConfig.h");
#endif
    delay(2000);
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(config.getWiFiSSID());

#ifdef ENABLE_LCD
  lcdManager.updateScreen("Connecting WiFi", "");
#endif

  WiFi.begin(config.getWiFiSSID(), config.getWiFiPassword());

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected! IP: ");
    Serial.println(WiFi.localIP());

#ifdef ENABLE_LCD
    lcdManager.updateScreen("WiFi OK", WiFi.localIP().toString().c_str());
#endif

#ifdef ENABLE_STATUS_LED
    ledManager.showWifiConnected();  // network up — not yet fully initialized
#endif

    Serial.println("Setting up NTP...");
    configTime(0, 0, "pool.ntp.org");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
#ifdef ENABLE_LCD
      lcdManager.updateScreen("NTP FAILED", "");
#endif
    } else {
      Serial.println("Time obtained");
    }

    delay(2000);
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");

#ifdef ENABLE_LCD
    lcdManager.updateScreen("WiFi FAILED", "");
#endif

#ifdef ENABLE_STATUS_LED
    ledManager.showWifiFailed();
#endif

    delay(2000);
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Starting setup ===");

#ifdef ENABLE_STATUS_LED
  ledManager.begin(PIN_STATUS_LED);
  ledManager.showBooting();
#endif

#ifdef ENABLE_LCD
  // Initialize I2C with custom pins for LCD
  Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);
  Serial.println("I2C initialized");

  // Initialize LCD and start its task on core 0
  lcdManager.begin();
  lcdManager.startTask();
  lcdManager.updateScreen("Initializing...", "");
  Serial.println("LCD initialized");
#endif

  // Initialize ConfigurationManager FIRST (loads NVS)
  if (!ConfigurationManager::getInstance().begin()) {
    Serial.println("ConfigurationManager init failed - halting");
#ifdef ENABLE_LCD
    lcdManager.updateScreen("Config FAILED", "");
#endif
    while (1) { delay(1000); }
  }
#ifdef ENABLE_LCD
  lcdManager.setScreenTimeoutMs(ConfigurationManager::getInstance().getLcdTimeoutMs());
#endif

  // Initialize ApplicationManager (message queue) with LCD reference
#ifdef ENABLE_LCD
  if (!ApplicationManager::getInstance().begin(&lcdManager)) {
#else
  if (!ApplicationManager::getInstance().begin(nullptr)) {
#endif
    Serial.println("ApplicationManager init failed - halting");
    while (1) { delay(1000); }
  }

  // Initialize BluetoothManager BEFORE WiFi (they share the radio)
#ifdef ENABLE_LCD
  lcdManager.updateScreen("Starting BLE...", "");
#endif
  if (!BluetoothManager::getInstance().begin()) {
    Serial.println("BluetoothManager init failed - continuing without BLE");
  }

  // Connect to WiFi
  initWiFi();

  // Create global HTTP mutex for serializing HTTP requests
  g_httpMutex = xSemaphoreCreateMutex();
  if (g_httpMutex == nullptr) {
    Serial.println("Failed to create HTTP mutex - halting");
    while (1) { delay(1000); }
  }

  // Initialize SpoolmanManager
  if (!SpoolmanManager::getInstance().begin(g_httpMutex)) {
    Serial.println("SpoolmanManager init failed - continuing without Spoolman");
  }

  // Initialize HomeAssistantManager
  if (!HomeAssistantManager::getInstance().begin()) {
    Serial.println("HomeAssistantManager init failed - continuing without HA");
  }

  // Load automation mode from config
  {
    uint8_t mode = ConfigurationManager::getInstance().getAutomationMode();
    ApplicationManager::getInstance().setAutomationMode(static_cast<AutomationMode>(mode));
    Serial.printf("Automation mode: %s\n",
                  mode == 0 ? "SELF_DIRECTED" : "CONTROLLED_BY_HA");
  }

  // Initialize NFCManager
  if (!NFCManager::getInstance().begin()) {
    Serial.println("NFCManager init failed - halting");
#ifdef ENABLE_LCD
    lcdManager.updateScreen("NFC FAILED", "");
#endif
    while (1) { delay(1000); }
  }

  // Start NFC scan task
  NFCManager::getInstance().startScanTask();

  // Start SpoolmanManager task
  SpoolmanManager::getInstance().startTask();

  // Start HomeAssistantManager task
  auto& config = ConfigurationManager::getInstance();
  Serial.printf("Setup: HA config before startTask: enabled=%s host='%s' host_len=%u port=%u user_set=%s\n",
                config.getHAEnabled() ? "true" : "false",
                config.getHAMqttHost(),
                static_cast<unsigned>(strlen(config.getHAMqttHost())),
                static_cast<unsigned>(config.getHAMqttPort()),
                strlen(config.getHAMqttUser()) > 0 ? "true" : "false");
  HomeAssistantManager::getInstance().startTask();

#ifdef ENABLE_LCD
  ApplicationManager::getInstance().showStatusOnLCD();
#endif

#ifdef ENABLE_STATUS_LED
  ledManager.showReady();  // NFC + Spoolman + HA + scanner all initialized
  ledManager.startTask();  // Start async LED task — all LED calls are non-blocking from here
#endif

  // Start HTTP tag writer server (WiFi must be connected)
  if (WiFi.status() == WL_CONNECTED) {
    WebServerManager::getInstance().begin();
  }

  Serial.println("=== Setup complete ===");
}

void loop() {
  // Process any pending messages for the application
  ApplicationManager::getInstance().processMessages();

  // Process HTTP requests for the tag writer web UI
  WebServerManager::getInstance().handleClient();

  // LCD and NFC scanning are handled by their own tasks
  delay(10);
}