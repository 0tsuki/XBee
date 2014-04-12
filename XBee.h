#ifndef XBeeClient_h
#define XBeeClient_h

#include "Arduino.h"

class XBeeAddress64
{
    public:
        XBeeAddress64(uint32_t address);
        uint32_t getAddress();
    private:
        uint32_t _address;
};

/*
class XBeeClient
{
    public:
        XBeeClient();
        void setApiMode(uint8_t apiMode = 2);
        void send(ApiFrame request);
    private:
        uint8_t _apiMode;
};

class ApiFrame
{
    public:
        ApiFrame();
};

class ZigBeeTransmitRequest : public ApiFrame
{
};
*/

#endif
