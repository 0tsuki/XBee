#ifndef XBee_h
#define XBee_h

#include "Arduino.h"
#include "HardwareSerial.h"

class XBeeAddress
{
    public:
        XBeeAddress();
        XBeeAddress(unsigned char* address);
        bool isEmpty();
        unsigned char* getAddress();
        unsigned char getAddress(int index);
    private:
        bool _isEmpty;
        unsigned char* _address;
};

class Request
{
    public:
        Request();
        void setRfData(int size, unsigned char* rfData);
        unsigned char calcChecksum();
        unsigned char* _rfData;
        int _rfDataSize;
};

class XBeeClient
{
    public:
        XBeeClient();
        void setSerial(int speed);
        void send(Request request, XBeeAddress address);
    private:
        int _apiMode;
        void write(unsigned char data);
        unsigned char calcChecksum(Request request, XBeeAddress address);
};

#endif
