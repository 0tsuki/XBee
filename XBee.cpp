#include "Arduino.h"
#include "XBee.h"

XBeeAddress64::XBeeAddress64(uint32_t address)
{
    _address = address;
}

uint32_t XBeeAddress64::getAddress()
{
    return _address;
}
