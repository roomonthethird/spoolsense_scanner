#include "InputManager.h"
#include "ApplicationManager.h"
#include "BoardPins.h"
#include <Arduino.h>

InputManager& InputManager::getInstance() {
    static InputManager instance;
    return instance;
}

InputManager::InputManager() {
    // Standard 3x4 matrix keypad layout
    keys_[0][0] = '1'; keys_[0][1] = '2'; keys_[0][2] = '3';
    keys_[1][0] = '4'; keys_[1][1] = '5'; keys_[1][2] = '6';
    keys_[2][0] = '7'; keys_[2][1] = '8'; keys_[2][2] = '9';
    keys_[3][0] = '*'; keys_[3][1] = '0'; keys_[3][2] = '#';

    rowPins_[0] = PIN_KEYPAD_ROW1;
    rowPins_[1] = PIN_KEYPAD_ROW2;
    rowPins_[2] = PIN_KEYPAD_ROW3;
    rowPins_[3] = PIN_KEYPAD_ROW4;

    colPins_[0] = PIN_KEYPAD_COL1;
    colPins_[1] = PIN_KEYPAD_COL2;
    colPins_[2] = PIN_KEYPAD_COL3;
}

void InputManager::begin() {
    if (initialized_) return;

    keypad_ = new Keypad(makeKeymap(keys_), rowPins_, colPins_, ROWS, COLS);
    keypad_->setDebounceTime(50);
    initialized_ = true;

    Serial.println("InputManager: 3x4 keypad initialized");
}

void InputManager::poll() {
    if (!initialized_ || !keypad_) return;

    char key = keypad_->getKey();
    if (key == 0) return;  // No key pressed

    Serial.printf("InputManager: key pressed '%c'\n", key);

    AppMessage msg;
    memset(&msg, 0, sizeof(msg));

    if (key >= '0' && key <= '9') {
        msg.type = AppMessageType::KEYPAD_DIGIT;
        msg.payload.keypadDigit.digit = key;
    } else if (key == '#') {
        msg.type = AppMessageType::KEYPAD_CONFIRM;
    } else if (key == '*') {
        msg.type = AppMessageType::KEYPAD_CANCEL;
    } else {
        return;  // Unknown key, ignore
    }

    ApplicationManager::getInstance().sendMessage(msg);
}
