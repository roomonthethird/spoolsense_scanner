#include <WiFi.h>
#include <time.h>
#include <cstring>

#include "ConfigurationManager.h"
#include "ApplicationManager.h"
#include "NFCManager.h"
#include "SpoolmanManager.h"
#include "HomeAssistantManager.h"
#include "LCDManager.h"
#include "LEDManager.h"
#include "WebServerManager.h"
#include "PrinterManager.h"
#include "PrusaLinkStrategy.h"
#include "InputManager.h"
#include "HardwareNFCConnectionPN532.h"
#include "BoardPins.h"
#include <Wire.h>

// Global HTTP mutex for serializing WiFi HTTP requests
SemaphoreHandle_t g_httpMutex = nullptr;

// PrusaLink strategy (file-scope so it outlives setup)
static PrusaLinkStrategy prusaLinkStrategy;

// Always declared; only initialized if isLcdEnabled() at runtime
LCDManager lcdManager(0x27, 16, 2);

// Always declared; only initialized if isLedEnabled() at runtime
LEDManager ledManager;

void initWiFi() {
  auto& config = ConfigurationManager::getInstance();

  if (strlen(config.getWiFiSSID()) == 0) {
    Serial.println("WiFi SSID not configured - skipping WiFi");
    if (config.isLcdEnabled()) {
      lcdManager.updateScreen("WiFi: no SSID", "Check UserConfig.h");
    }
    delay(2000);
    return;
  }

  Serial.print("Connecting to WiFi: ");
  Serial.println(config.getWiFiSSID());

  if (config.isLcdEnabled()) {
    lcdManager.updateScreen("Connecting WiFi", "");
  }

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

    if (config.isLcdEnabled()) {
      lcdManager.updateScreen("WiFi OK", WiFi.localIP().toString().c_str());
    }

    if (config.isLedEnabled()) {
      ledManager.showWifiConnected();  // network up — not yet fully initialized
    }

    Serial.println("Setting up NTP...");
    configTime(0, 0, "pool.ntp.org");
    struct tm timeinfo;
    if (!getLocalTime(&timeinfo)) {
      Serial.println("Failed to obtain time");
      if (config.isLcdEnabled()) {
        lcdManager.updateScreen("NTP FAILED", "");
      }
    } else {
      Serial.println("Time obtained");
    }

    delay(2000);
  } else {
    Serial.println("");
    Serial.println("WiFi connection failed!");

    if (config.isLcdEnabled()) {
      lcdManager.updateScreen("WiFi FAILED", "");
    }

    if (config.isLedEnabled()) {
      ledManager.showWifiFailed();
    }

    delay(2000);
  }
}

void setup() {
  delay(1000);
  Serial.begin(115200);
  delay(1000);
  Serial.println("=== Starting setup ===");

  // Initialize ConfigurationManager FIRST — reads NVS to determine which
  // optional hardware features are enabled before initializing any peripherals
  if (!ConfigurationManager::getInstance().begin()) {
    Serial.println("ConfigurationManager init failed - halting");
    while (1) { delay(1000); }
  }

  auto& config = ConfigurationManager::getInstance();

  if (config.isLedEnabled()) {
    ledManager.begin(PIN_STATUS_LED);
    ledManager.showBooting();
  }

  if (config.isLcdEnabled()) {
    // Initialize I2C with custom pins for LCD
    Wire.begin(PIN_LCD_SDA, PIN_LCD_SCL);
    Serial.println("I2C initialized");

    lcdManager.begin();
    lcdManager.startTask();
    lcdManager.updateScreen("Initializing...", "");
    Serial.println("LCD initialized");

    lcdManager.setScreenTimeoutMs(config.getLcdTimeoutMs());
  }

  if (config.isKeypadEnabled()) {
    InputManager::getInstance().begin();
  }

  // Initialize ApplicationManager (message queue) with LCD reference
  if (!ApplicationManager::getInstance().begin(config.isLcdEnabled() ? &lcdManager : nullptr)) {
    Serial.println("ApplicationManager init failed - halting");
    while (1) { delay(1000); }
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
    uint8_t mode = config.getAutomationMode();
    ApplicationManager::getInstance().setAutomationMode(static_cast<AutomationMode>(mode));
    Serial.printf("Automation mode: %s\n",
                  mode == 0 ? "SELF_DIRECTED" : "CONTROLLED_BY_HA");
  }

  // Select NFC reader based on NVS config
  const char* nfcReader = config.getNfcReader();
  if (strcmp(nfcReader, "pn532") == 0) {
    Serial.println("NFC reader: PN532 (ISO14443A only)");
    NFCManager::getInstance().setConnection(new HardwareNFCConnectionPN532());
  } else {
    Serial.printf("NFC reader: %s (default)\n", nfcReader);
  }

  // Initialize NFCManager
  if (!NFCManager::getInstance().begin()) {
    Serial.println("NFCManager init failed - halting");
    if (config.isLcdEnabled()) {
      lcdManager.updateScreen("NFC FAILED", "");
    }
    while (1) { delay(1000); }
  }

  // Start NFC scan task
  NFCManager::getInstance().startScanTask();

  // Start SpoolmanManager task
  SpoolmanManager::getInstance().startTask();

  // Start HomeAssistantManager task
  Serial.printf("Setup: HA config before startTask: enabled=%s host='%s' host_len=%u port=%u user_set=%s\n",
                config.getHAEnabled() ? "true" : "false",
                config.getHAMqttHost(),
                static_cast<unsigned>(strlen(config.getHAMqttHost())),
                static_cast<unsigned>(config.getHAMqttPort()),
                strlen(config.getHAMqttUser()) > 0 ? "true" : "false");
  HomeAssistantManager::getInstance().startTask();

  // Start PrusaLink printer polling (if configured)
  if (config.isPrusaLinkEnabled()) {
    PrinterManager::getInstance().begin();
    prusaLinkStrategy.setHttpMutex(g_httpMutex);
    PrinterManager::getInstance().setStrategy(&prusaLinkStrategy);
    PrinterManager::getInstance().startPollingTask();
    Serial.println("PrusaLink integration enabled");
  } else {
    Serial.println("PrusaLink integration disabled (not configured)");
  }

  if (config.isLcdEnabled()) {
    ApplicationManager::getInstance().showStatusOnLCD();
  }

  if (config.isLedEnabled()) {
    ledManager.showReady();  // NFC + Spoolman + HA + scanner all initialized
    ledManager.startTask();  // Start async LED task — all LED calls are non-blocking from here
  }

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

  // Poll keypad for key presses (if enabled)
  if (ConfigurationManager::getInstance().isKeypadEnabled()) {
    InputManager::getInstance().poll();
  }

  // LCD and NFC scanning are handled by their own tasks
  delay(10);
}
