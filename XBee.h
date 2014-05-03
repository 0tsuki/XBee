#ifndef XBee_h
#define XBee_h

#include "Arduino.h"
#include "HardwareSerial.h"

class XBeeAddress
{
    public:
        XBeeAddress();
        XBeeAddress(unsigned char address[8]);
        bool isEmpty();
        bool equal(XBeeAddress address);
        unsigned char* getAddress();
        unsigned char getAddress(int index);
    private:
        bool _isEmpty;
        unsigned char _address[8];
};

class Request
{
    public:
        Request();
        void setRfData(int size, unsigned char* rfData);
        int getRfDataSize();
        unsigned char* getRfData();
        unsigned char getRfData(int index);
        unsigned char calcChecksum();
    private:
        unsigned char* _rfData;
        int _rfDataSize;
};

class Response
{
    public:
        Response();
        void setData(unsigned char* packets);
        XBeeAddress getAddress();
        unsigned char* getRfData();
        bool isEmpty();
    private:
        bool _isEmpty;
        unsigned char _startDelimiter;
        unsigned char _length[2];
        unsigned char _frameType;
        XBeeAddress _address64;
        unsigned char _address16[2];
        unsigned char _options;
        unsigned char* _rfData;
        unsigned char _checksum;
};

class XBeeClient
{
    public:
        XBeeClient();
        void setSerial(int speed);
        int available();
        void send(Request request, XBeeAddress address);
        Response getResponse();
    private:
        int _apiMode;
        void write(unsigned char data);
        unsigned char readPacket();
        unsigned char calcLengthLSB(Request request);
        unsigned char calcChecksum(Request request, XBeeAddress address);
};

#endif
