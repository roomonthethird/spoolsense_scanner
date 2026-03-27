#ifndef INPUT_MANAGER_H
#define INPUT_MANAGER_H

#include <Keypad.h>

class InputManager {
public:
    static InputManager& getInstance();

    void begin();        // Configure keypad pins
    void poll();         // Check for key presses, enqueue messages (call from loop)

private:
    InputManager();
    InputManager(const InputManager&) = delete;
    InputManager& operator=(const InputManager&) = delete;

    static constexpr byte ROWS = 4;
    static constexpr byte COLS = 3;

    char keys_[ROWS][COLS];
    byte rowPins_[ROWS];
    byte colPins_[COLS];
    Keypad* keypad_ = nullptr;
    bool initialized_ = false;
};

#endif // INPUT_MANAGER_H
