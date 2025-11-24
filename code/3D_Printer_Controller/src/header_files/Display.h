#pragma once // Ensures the header is included only once when included multiple times

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET    -1 // Reset pin # (or -1 if sharing Arduino reset pin)

class Display : public Adafruit_SSD1306
{
public:
    Display();
    void printText(String text, int textSize = 1, bool clear = false);


};
