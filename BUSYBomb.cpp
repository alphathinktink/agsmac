//------------------------------------------------------------------------------------------
// BUSYBomb.cpp
#include "BUSYBomb.h"
#include "Arduino_GigaDisplay.h" // Include the GigaDisplay library
//------------------------------------------------------------------------------------------

static int counter = 0; // Initialize static counter
static GigaDisplayRGB rgb; // Initialize the RGB controller

//------------------------------------------------------------------------------------------
_BUSYBomb::_BUSYBomb()
{
    if (counter == 0)
    {
        rgb.begin(); // Initialize the RGB LED driver
    }
    counter++;
    turnOnLED();
}
//------------------------------------------------------------------------------------------
_BUSYBomb::~_BUSYBomb()
{
    counter--;
    turnOffLED();
}
//------------------------------------------------------------------------------------------
void _BUSYBomb::turnOnLED()
{
    if (counter == 1)
    {
        rgb.on(255, 0, 0); // Turn on the LED with red color
    }
}
//------------------------------------------------------------------------------------------
void _BUSYBomb::turnOffLED()
{
    if (counter == 0)
    {
        rgb.off(); // Turn off the LED
    }
}
//------------------------------------------------------------------------------------------
