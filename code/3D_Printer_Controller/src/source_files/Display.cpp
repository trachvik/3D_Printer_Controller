#include "header_files/Display.h"

Display::Display()
  : Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET)
{
}

void Display::printText(String text, int textSize, bool clear)
{
    if(clear)
    {
        this->clearDisplay();
    }
    this->setTextSize(textSize);
    this->setTextColor(SSD1306_WHITE);
    //this->setCursor(x, y);
    this->print(text);
    this->display();
}
