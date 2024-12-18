//------------------------------------------------------------------------------------------
#ifndef BUSYBOMB_H
#define BUSYBOMB_H
//------------------------------------------------------------------------------------------

#include <Arduino.h>

//------------------------------------------------------------------------------------------
class _BUSYBomb
{
private:
    void turnOnLED();
    void turnOffLED();

public:
    _BUSYBomb();  // Constructor: Turns on the LED if needed
    ~_BUSYBomb(); // Destructor: Turns off the LED if needed
};
//------------------------------------------------------------------------------------------
// Generate a unique name using the __LINE__ macro
#define __BUSYBomb(line) __BUSYBomb##line
#define __BUSYBombUnique(line) __BUSYBomb(line)
#define BUSYBomb _BUSYBomb __BUSYBombUnique(__LINE__);
//------------------------------------------------------------------------------------------
#endif // BUSYBOMB_H
//------------------------------------------------------------------------------------------
