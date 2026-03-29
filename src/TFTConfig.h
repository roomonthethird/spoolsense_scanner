#pragma once

#ifdef ENABLE_TFT

// TFT and LCD are mutually exclusive on WROOM (shared GPIO 22/23)
#if defined(ENABLE_LCD) && ENABLE_LCD == 1 && !defined(BOARD_ESP32_S3)
  #error "ENABLE_TFT and ENABLE_LCD cannot both be enabled on WROOM (GPIO 22/23 conflict)"
#endif

#include <LovyanGFX.hpp>
#include "BoardPins.h"

// ---------------------------------------------------------------------------
// LovyanGFX display config — one class per board, selected at compile time.
//
// Tested target: 240x240 ST7789 (common cheap round/square TFT).
// To use a different driver (ILI9341, GC9A01, etc.) change the _panel_instance
// type and adjust _width/_height below. Everything else stays the same.
//
// SPI bus used:
//   WROOM  — VSPI (bus 3): pins 18/23 freed from LCD when ENABLE_TFT replaces LCD
//   S3-Zero — SPI2 (FSPI): pins 13/15/16 (PN5180 is on separate SPI instance)
// ---------------------------------------------------------------------------

#if defined(BOARD_ESP32_S3)

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX() {
        // SPI bus
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host   = SPI2_HOST;  // FSPI on S3
            cfg.spi_mode   = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read  = 16000000;
            cfg.pin_sclk   = PIN_TFT_SCLK;
            cfg.pin_mosi   = PIN_TFT_MOSI;
            cfg.pin_miso   = PIN_TFT_MISO; // -1 if not connected
            cfg.pin_dc     = PIN_TFT_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = PIN_TFT_CS;
            cfg.pin_rst  = PIN_TFT_RST;
            cfg.pin_busy = -1;
            cfg.memory_width  = 240;
            cfg.memory_height = 240;
            cfg.panel_width   = 240;
            cfg.panel_height  = 240;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable     = false;
            cfg.invert       = true;  // ST7789 typically needs invert=true
            cfg.rgb_order    = false;
            cfg.dlen_16bit   = false;
            cfg.bus_shared   = false;
            _panel_instance.config(cfg);
        }

        // Backlight
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = PIN_TFT_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

#else // WROOM

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789  _panel_instance;
    lgfx::Bus_SPI       _bus_instance;
    lgfx::Light_PWM     _light_instance;

public:
    LGFX() {
        // SPI bus — VSPI (bus 3), freed from LCD when TFT enabled
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host   = VSPI_HOST;
            cfg.spi_mode   = 0;
            cfg.freq_write = 40000000;
            cfg.freq_read  = 16000000;
            cfg.pin_sclk   = PIN_TFT_SCLK;
            cfg.pin_mosi   = PIN_TFT_MOSI;
            cfg.pin_miso   = PIN_TFT_MISO;
            cfg.pin_dc     = PIN_TFT_DC;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }

        // Panel
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs   = PIN_TFT_CS;
            cfg.pin_rst  = PIN_TFT_RST;
            cfg.pin_busy = -1;
            cfg.memory_width  = 240;
            cfg.memory_height = 240;
            cfg.panel_width   = 240;
            cfg.panel_height  = 240;
            cfg.offset_x      = 0;
            cfg.offset_y      = 0;
            cfg.offset_rotation = 0;
            cfg.dummy_read_pixel = 8;
            cfg.dummy_read_bits  = 1;
            cfg.readable     = false;
            cfg.invert       = true;
            cfg.rgb_order    = false;
            cfg.dlen_16bit   = false;
            cfg.bus_shared   = false;
            _panel_instance.config(cfg);
        }

        // Backlight
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = PIN_TFT_BL;
            cfg.invert      = false;
            cfg.freq        = 44100;
            cfg.pwm_channel = 7;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }

        setPanel(&_panel_instance);
    }
};

#endif // BOARD_ESP32_S3
#endif // ENABLE_TFT
