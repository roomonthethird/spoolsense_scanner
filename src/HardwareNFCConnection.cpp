#include "HardwareNFCConnection.h"
#include <Arduino.h>
#include <cstring>

//#define ENABLE_NFC_DEBUG_LOGS
static const uint8_t openprinttag_Sunlu_ASA_bin[] = {
  0xe1, 0x40, 0x27, 0x01, 0x03, 0xff, 0x01, 0x2f, 0xc2, 0x1c, 0x00, 0x00,
  0x01, 0x0d, 0x61, 0x70, 0x70, 0x6c, 0x69, 0x63, 0x61, 0x74, 0x69, 0x6f,
  0x6e, 0x2f, 0x76, 0x6e, 0x64, 0x2e, 0x6f, 0x70, 0x65, 0x6e, 0x70, 0x72,
  0x69, 0x6e, 0x74, 0x74, 0x61, 0x67, 0xa1, 0x02, 0x18, 0xea, 0xbf, 0x08,
  0x00, 0x09, 0x04, 0x0a, 0x69, 0x53, 0x75, 0x6e, 0x6c, 0x75, 0x20, 0x41,
  0x53, 0x41, 0x0b, 0x65, 0x53, 0x75, 0x6e, 0x6c, 0x75, 0x0e, 0x1a, 0x69,
  0xb4, 0xa5, 0x00, 0x10, 0x19, 0x03, 0xe8, 0x11, 0x19, 0x04, 0xb0, 0x12,
  0x18, 0xc8, 0x13, 0x43, 0xfc, 0x03, 0x03, 0x18, 0x1d, 0xf9, 0x3c, 0x29,
  0x18, 0x22, 0x18, 0xf5, 0x18, 0x23, 0x19, 0x01, 0x09, 0x18, 0x25, 0x18,
  0x50, 0x18, 0x26, 0x18, 0x64, 0x18, 0x37, 0x62, 0x55, 0x53, 0x18, 0x39,
  0x18, 0x3c, 0x18, 0x3a, 0x19, 0x01, 0x2c, 0xff, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0xa0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xfe
};

static constexpr size_t openprinttag_Sunlu_ASA_bin_len = 312;
HardwareNFCConnection::HardwareNFCConnection() {
    memset(currentUid_, 0, sizeof(currentUid_));
    memset(&hal_, 0, sizeof(hal_));
}

HardwareNFCConnection::~HardwareNFCConnection() {
    delete nfc_;
    delete iso14443a_;
}

opt_error_t HardwareNFCConnection::halReadPage(void* ctx, uint8_t page, uint8_t* buffer) {
    HardwareNFCConnection* self = static_cast<HardwareNFCConnection*>(ctx);
    ISO15693ErrorCode err = self->nfc_->readSingleBlock(self->currentUid_, page, buffer, 4);
    return (err == ISO15693_EC_OK) ? OPT_OK : OPT_ERR_NFC_READ;
}

opt_error_t HardwareNFCConnection::halWritePage(void* ctx, uint8_t page, const uint8_t* data) {
    HardwareNFCConnection* self = static_cast<HardwareNFCConnection*>(ctx);
    ISO15693ErrorCode err = self->nfc_->writeSingleBlock(self->currentUid_, page, const_cast<uint8_t*>(data), 4);
    if (err != ISO15693_EC_OK) {
        Serial.printf("HardwareNFC: writeSingleBlock failed on page %d: %s (0x%02X)\n",
                 page, self->nfc_->strerror(err), (int)err);
        return OPT_ERR_NFC_WRITE;
    }
    return OPT_OK;
}

bool HardwareNFCConnection::begin() {
    // Configure additional input pins for future use
    pinMode(PN5180_IRQ, INPUT);    // Interrupt (active HIGH)
    pinMode(PN5180_GPIO, INPUT);   // Card detection
    pinMode(PN5180_AUX, INPUT);    // Auxiliary/monitoring

    nfc_ = new PN5180ISO15693(PN5180_NSS, PN5180_BUSY, PN5180_RST,
                               PN5180_SCK, PN5180_MISO, PN5180_MOSI);
    iso14443a_ = new PN5180ISO14443(PN5180_NSS, PN5180_BUSY, PN5180_RST,
                                    PN5180_SCK, PN5180_MISO, PN5180_MOSI);

    Serial.println("HardwareNFCConnection: Starting PN5180...");
    nfc_->begin();
    iso14443a_->begin();
    Serial.println("HardwareNFCConnection: SPI begin done, resetting...");
    Serial.printf("HardwareNFCConnection: BUSY pin=%d before reset\n", digitalRead(PN5180_BUSY));

    // Manual reset with debug (PN5180::reset() has no timeout)
    digitalWrite(PN5180_RST, LOW);
    delay(10);
    digitalWrite(PN5180_RST, HIGH);
    Serial.println("HardwareNFCConnection: RST pin released, waiting for boot...");

    // Wait for BUSY to go LOW with timeout (chip boots with RF subsystem)
    unsigned long start = millis();
    while (digitalRead(PN5180_BUSY) == HIGH) {
        if (millis() - start > 2000) {
            Serial.println("HardwareNFCConnection: TIMEOUT waiting for BUSY LOW after reset!");
            break;
        }
        delay(1);
    }
    Serial.printf("HardwareNFCConnection: BUSY went LOW after %lums\n", millis() - start);

    // Wait for IDLE IRQ with timeout
    start = millis();
    uint32_t irqStatus = 0;
    while (0 == (irqStatus & (1 << 2))) {  // IDLE_IRQ_STAT
        nfc_->readRegister(IRQ_STATUS, &irqStatus);
        if (millis() - start > 2000) {
            Serial.printf("HardwareNFCConnection: TIMEOUT waiting for IDLE IRQ! IRQ=0x%08lX\n", irqStatus);
            break;
        }
        delay(1);
    }
    Serial.printf("HardwareNFCConnection: IDLE IRQ after %lums, IRQ=0x%08lX\n", millis() - start, irqStatus);
    nfc_->clearIRQStatus(0xffffffff);
    Serial.println("HardwareNFCConnection: Reset complete");

    // Read firmware version
    uint8_t firmwareVersion[2];
    nfc_->readEEprom(PN5180_FIRMWARE_VERSION, firmwareVersion, 2);
    Serial.printf("HardwareNFCConnection: PN5180 firmware: %d.%d\n",
                  firmwareVersion[1], firmwareVersion[0]);

    // Setup RF for ISO15693
    if (!nfc_->setupRF()) {
        Serial.println("HardwareNFCConnection: Failed to setup RF");
        return false;
    }

    // Set up HAL for openprinttag
    hal_.read_page = halReadPage;
    hal_.write_page = halWritePage;
    hal_.is_present = nullptr;
    hal_.user_ctx = this;

    Serial.println("HardwareNFCConnection: Initialized successfully");
    return true;
}

void HardwareNFCConnection::reset() {
    if (nfc_) {
        nfc_->reset();
    }
}

bool HardwareNFCConnection::hardwareReset() {
    if (!nfc_) return false;

    Serial.println("HardwareNFC: hardwareReset() - toggling RST pin");

    // Toggle RST pin to force full hardware reset
    digitalWrite(PN5180_RST, LOW);
    delay(10);
    digitalWrite(PN5180_RST, HIGH);

    // Wait for BUSY to go LOW with timeout
    unsigned long start = millis();
    while (digitalRead(PN5180_BUSY) == HIGH) {
        if (millis() - start > 2000) {
            Serial.println("HardwareNFC: hardwareReset TIMEOUT waiting for BUSY LOW");
            return false;
        }
        delay(1);
    }

    // Wait for IDLE IRQ with timeout
    start = millis();
    uint32_t irqStatus = 0;
    while (0 == (irqStatus & IDLE_IRQ_STAT)) {
        nfc_->readRegister(IRQ_STATUS, &irqStatus);
        if (millis() - start > 2000) {
            Serial.printf("HardwareNFC: hardwareReset TIMEOUT waiting for IDLE IRQ, IRQ=0x%08lX\n", irqStatus);
            return false;
        }
        delay(1);
    }
    nfc_->clearIRQStatus(0xffffffff);

    // Re-setup RF
    return setupRF();
}

bool HardwareNFCConnection::setupRF() {
    if (!nfc_) return false;

    if (!nfc_->loadRFConfig(0x0d, 0x8d)) return false;
    if (!nfc_->setRF_on()) return false;

    // not sure this is true, taking out:
    // This sequence is critical. After turning the RF field on, the PN5180
    // must be explicitly put into the Idle state and then the Transceive
    // state. This prepares the chip's internal state machine to handle
    // subsequent data transmission commands reliably. Omitting this leads
    // to intermittent failures on subsequent tag reads.
    //nfc_->writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle/StopCom
    //nfc_->writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);   // Transceive

    return true;
}

bool HardwareNFCConnection::detectTag(uint8_t* uid, uint8_t* uidLength) {
    if (!nfc_) {
        Serial.println("HardwareNFC: detectTag() called but nfc_ is null!");
        return false;
    }

    ISO15693ErrorCode err = nfc_->getInventory(uid);
    if (err == ISO15693_EC_OK) {
        // Reject phantom detections (all-zero UID = tag not fully powered)
        bool allZero = true;
        for (int i = 0; i < 8; i++) {
            if (uid[i] != 0) { allZero = false; break; }
        }
        if (allZero) return false;

        *uidLength = 8;  // ISO15693 uses 8-byte UID
        return true;
    }

    // ISO15693 not found — try ISO14443A (NTAG215 etc.)
    if (iso14443a_) {
        iso14443a_->setupRF();
        uint8_t iso14443aBuf[7] = {0};
        uint8_t iso14443aLen = iso14443a_->readCardSerial(iso14443aBuf);
        if (iso14443aLen >= 4) {
            memcpy(uid, iso14443aBuf, iso14443aLen);
            *uidLength = iso14443aLen;
            return true;
        }
    }

    // Log non-OK errors periodically (not every poll to avoid spam)
    static uint32_t errCount = 0;
    errCount++;
    if (errCount % 200 == 1) {
        Serial.printf("HardwareNFC: getInventory err=%d (%s) [count=%lu]\n",
                      (int)err, nfc_->strerror(err), errCount);
    }
    return false;
}

void HardwareNFCConnection::setCurrentUid(const uint8_t* uid, uint8_t length) {
    memcpy(currentUid_, uid, length < 8 ? length : 8);
}

opt_nfc_hal_t* HardwareNFCConnection::getHal() {
    return &hal_;
}
bool HardwareNFCConnection::writeTestOpenPrintTag() {
    if (!nfc_) {
        Serial.println("HardwareNFC: Cannot write test tag, nfc_ is null");
        return false;
    }

    bool uidSet = false;
    for (uint8_t b : currentUid_) {
        if (b != 0) {
            uidSet = true;
            break;
        }
    }
    if (!uidSet) {
        Serial.println("HardwareNFC: No current tag UID set, cannot write test tag");
        return false;
    }

    if ((openprinttag_Sunlu_ASA_bin_len % 4) != 0) {
        Serial.println("HardwareNFC: Test tag payload is not 4-byte block aligned");
        return false;
    }

    const size_t blockCount = openprinttag_Sunlu_ASA_bin_len / 4;
    Serial.printf("HardwareNFC: Writing test OpenPrintTag payload (%u blocks)\n",
                  static_cast<unsigned>(blockCount));

    for (size_t block = 0; block < blockCount; ++block) {
        const uint8_t* data = &openprinttag_Sunlu_ASA_bin[block * 4];
        opt_error_t err = halWritePage(this, static_cast<uint8_t>(block), data);
        if (err != OPT_OK) {
            Serial.printf("HardwareNFC: Failed writing test payload block %u\n",
                          static_cast<unsigned>(block));
            return false;
        }
    }

    Serial.println("HardwareNFC: Test OpenPrintTag payload written successfully");
    return true;
}

void HardwareNFCConnection::logDiagnostics() {
    if (!nfc_) {
        Serial.println("HardwareNFC DIAG: nfc_ is null!");
        return;
    }

    uint32_t irqStatus, rfStatus, sysStatus;
    nfc_->readRegister(IRQ_STATUS, &irqStatus);
    nfc_->readRegister(RF_STATUS, &rfStatus);
    nfc_->readRegister(SYSTEM_STATUS, &sysStatus);

    uint8_t transceiverState = (rfStatus >> 24) & 0x07;
    bool rfFieldOn = (rfStatus & 0x01);  // TX_RF_STATUS bit
    bool extFieldDet = (rfStatus & 0x02);  // RF_DET_STATUS bit

    #ifdef ENABLE_NFC_DEBUG_LOGS

        Serial.printf("HardwareNFC DIAG: IRQ=0x%08lX RF=0x%08lX SYS=0x%08lX\n",
                    irqStatus, rfStatus, sysStatus);
        Serial.printf("HardwareNFC DIAG: RF_field=%s ext_field=%s transceiver=%u\n",
                    rfFieldOn ? "ON" : "OFF",
                    extFieldDet ? "YES" : "NO",
                    transceiverState);

        // Step-by-step RF activation test
        Serial.println("HardwareNFC DIAG: --- RF activation test ---");

    #endif
    

    // Step 1: Reset and check
    nfc_->reset();
    nfc_->readRegister(RF_STATUS, &rfStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After reset: RF=0x%08lX field=%s\n",
                    rfStatus, (rfStatus & 0x01) ? "ON" : "OFF");
    #endif


    // Step 2: Load RF config
    nfc_->loadRFConfig(0x0d, 0x8d);
    nfc_->readRegister(RF_STATUS, &rfStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After loadRFConfig: RF=0x%08lX field=%s\n",
                    rfStatus, (rfStatus & 0x01) ? "ON" : "OFF");
    #endif


    // Step 3: Turn RF on
    nfc_->setRF_on();
    nfc_->readRegister(RF_STATUS, &rfStatus);
    nfc_->readRegister(IRQ_STATUS, &irqStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After setRF_on: RF=0x%08lX IRQ=0x%08lX field=%s\n",
                    rfStatus, irqStatus, (rfStatus & 0x01) ? "ON" : "OFF");
    #endif

    // Step 4: Wait and check again
    delay(50);
    nfc_->readRegister(RF_STATUS, &rfStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After 50ms wait: RF=0x%08lX field=%s\n",
                    rfStatus, (rfStatus & 0x01) ? "ON" : "OFF");
    #endif

    // Step 5: Set transceive (like setupRF does) and check
    nfc_->writeRegisterWithAndMask(SYSTEM_CONFIG, 0xfffffff8);  // Idle
    nfc_->readRegister(RF_STATUS, &rfStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After Idle cmd: RF=0x%08lX field=%s\n",
                    rfStatus, (rfStatus & 0x01) ? "ON" : "OFF");
    #endif


    nfc_->writeRegisterWithOrMask(SYSTEM_CONFIG, 0x00000003);  // Transceive
    nfc_->readRegister(RF_STATUS, &rfStatus);

    #ifdef ENABLE_NFC_DEBUG_LOGS
        Serial.printf("HardwareNFC DIAG: After Transceive cmd: RF=0x%08lX field=%s\n",
                    rfStatus, (rfStatus & 0x01) ? "ON" : "OFF");

        Serial.println("HardwareNFC DIAG: --- end test ---");
    #endif
}
