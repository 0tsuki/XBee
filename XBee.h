#ifndef XBee_h
#define XBee_h

#include "Arduino.h"
#include "HardwareSerial.h"

/*
 * API Frame constants
 */
#define AT_COMMAND_REQUEST 0x08
#define AT_COMMAND_RESPONSE 0x88
#define TRANSMIT_REQUEST 0x10
#define REMOTE_AT_COMMAND_REQUEST 0x17

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

class BaseRequest
{
public:
    unsigned char getFrameType();
    void setFrameType(unsigned char frameType);
    unsigned char getFrameId();
    void setFrameId(unsigned char frameId);
    unsigned char checkSum();

    virtual unsigned char getLsb() = 0;
    virtual unsigned char getFrameData(unsigned char position) = 0;

private:
    unsigned char _frameType;
    unsigned char _frameId;
};

class AtCommandRequest : public BaseRequest
{
public:
    AtCommandRequest(unsigned char* command);
    unsigned char* getCommand();
    unsigned char getCommandLength();
    unsigned char getLsb();
    unsigned char getFrameData(unsigned char position);

private:
    unsigned char* _command;
    unsigned char* _parameter;
    unsigned char _commandLength;
};

class Request
{
    public:
        Request();
        void setRfData(int size, unsigned char* rfData);
        int getRfDataSize();
        unsigned char* getRfData();
        unsigned char getRfData(int index);
    private:
        unsigned char* _rfData;
        int _rfDataSize;
};

class RemoteATCommandRequest
{
  public:
    RemoteATCommandRequest();
    XBeeAddress xbeeAddress;
    unsigned char atCommand[2];
    unsigned char commandParameter;
    unsigned char remoteCommandOptions;
    unsigned char getLengthLSB();
    unsigned char getFrameType();
    unsigned char getFrameId();
    unsigned char* getFrameData();
    unsigned char getChecksum();
  private:
    unsigned char _lengthLSB;
    unsigned char _frameType;
    unsigned char _frameId;
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

class BaseResponse
{
public:
    unsigned char getMsb();
    unsigned char getLsb();
    unsigned char getFrameType();
    unsigned char getFrameId();
    unsigned char getChecksum();
    void setMsb(unsigned char msb);
    void setChecksum(unsigned char checksum);
    void setFrameId(unsigned char frameId);
    void setFrameType(unsigned char frameType);
    void setLsb(unsigned char lsb);

private:
    unsigned char msb;
    unsigned char lsb;
    unsigned char frameType;
    unsigned char frameId;
    unsigned char checksum;
};

class AtCommandResponse : public BaseResponse
{
public:
    AtCommandResponse();
    unsigned char command[2];
    unsigned char commandStatus;
    unsigned char* commandData;
};

class XBeeClient
{
public:
    XBeeClient();
    void setSerial(int speed);
    int available();
    void send(AtCommandRequest request);
    void send(Request request, XBeeAddress address);
    void send(RemoteATCommandRequest request);
    Response getResponse();
    void readResponse(AtCommandResponse* response);

private:
    int _apiMode;
    void write(unsigned char data);
    unsigned char readPacket();
    unsigned char calcLengthLSB(Request request);
    unsigned char calcChecksum(Request request, XBeeAddress address);
};

#endif
