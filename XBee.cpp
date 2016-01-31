#include <stddef.h>
#include "Arduino.h"
#include "XBee.h"

const unsigned char START_DELIMITER = 0x7E;
const unsigned char ESCAPE_CHARACTER = 0x7D;
const unsigned char XON_CHARACTER = 0x11;
const unsigned char XOFF_CHARACTER = 0x13;

const unsigned char LENGTH_MSB = 0x00;
const unsigned char FRAME_ID = 0x00;
const unsigned char RESERVED[2] = {0xFF, 0xFE};
const unsigned char BROADCAST_REDIUS = 0x00;
const unsigned char TRANSMIT_OPTION = 0x00;

XBeeAddress::XBeeAddress()
{
    _isEmpty = true;
    for (int i = 0; i < 8; i++) {
        _address[i] = 0x00;
    }
}

XBeeAddress::XBeeAddress(unsigned char addr[8])
{
    _isEmpty = false;
    for (int i = 0; i < 8; i++) {
        _address[i] = addr[i];
    }
}

bool XBeeAddress::isEmpty()
{
    return _isEmpty;
}

bool XBeeAddress::equal(XBeeAddress address)
{
    for (int i = 0; i < 8; i++) {
        if (_address[i] != address.getAddress(i)) {
            return false;
        }
    }
    return true;
}

unsigned char* XBeeAddress::getAddress()
{
    return _address;
}

unsigned char XBeeAddress::getAddress(int index)
{
    return _address[index];
}

unsigned char BaseRequest::getFrameType() { return _frameType; }
void BaseRequest::setFrameType(unsigned char frameType) { _frameType = frameType; }
unsigned char BaseRequest::getFrameId() { return _frameId; }
void BaseRequest::setFrameId(unsigned char frameId) { _frameId = frameId; }

unsigned char BaseRequest::checkSum()
{
    unsigned char sum = 0;
    for (int i = 0; i < getLsb(); i++) {
        sum += getFrameData(i);
    }
    return (unsigned char) (0xFF - sum);
}

AtCommandRequest::AtCommandRequest(unsigned char *command) {
    setFrameType(AT_COMMAND_REQUEST);
    setFrameId(0x01);
    _command = command;
    _parameter = NULL;
    _commandLength = 0x02;
}

unsigned char* AtCommandRequest::getCommand() {
    return _command;
}

unsigned char AtCommandRequest::getCommandLength() {
    return _commandLength;
}

unsigned char AtCommandRequest::getLsb() {
    return (unsigned char) (0x02 + getCommandLength());
}

unsigned char AtCommandRequest::getFrameData(unsigned char position) {
    if (position == 0) {
        return getFrameType();
    } else if (position == 1) {
        return getFrameId();
    } else if (position == 2 || position == 3) {
        return _command[position - 2];
    } else {
        return _parameter[position - 4];
    }
}

Request::Request()
{
}

void Request::setRfData(int size, unsigned char* rfData)
{
    _rfDataSize = size;
    _rfData = rfData;
}

unsigned char* Request::getRfData()
{
    return _rfData;
}

unsigned char Request::getRfData(int index)
{
    return _rfData[index];
}

int Request::getRfDataSize()
{
    return _rfDataSize;
}

RemoteATCommandRequest::RemoteATCommandRequest()
{
    setFrameType(REMOTE_AT_COMMAND_REQUEST);
    setFrameId(0x01);
    reserved[0] = 0xFF;
    reserved[1] = 0xFE;
    options = 0x02;
    parameter = NULL;
    commandLength = 0x02;
}

unsigned char RemoteATCommandRequest::getLsb() {
    if (parameter == NULL) {
        return 0x0F;
    } else {
        return 0xFF;  // TODO
    }
}

unsigned char RemoteATCommandRequest::getFrameData(unsigned char position) {
    if (position == 0) {
        return getFrameType();
    } else if (position == 1) {
        return getFrameId();
    } else if (position >= 2 && position <= 9) {
        return xbeeAddress.getAddress(position - 2);
    } else if (position == 10 || position == 11) {
        return reserved[position - 10];
    } else if (position == 12) {
        return options;
    } else if (position == 13 || position == 14) {
        return command[position - 13];
    } else if (position == 15){
        if (parameter == NULL) {
           return NULL;
        } else {
            return parameter[0];
        }
    } else {
        return 0xFF;
    }
}

Response::Response()
{
    _isEmpty = true;
}

void Response::setData(unsigned char* data)
{
    _isEmpty = false;

    _startDelimiter = data[0];
    _length[0] = data[1];
    _length[1] = data[2];
    _frameType = data[3];

    unsigned char address64[8] = {
        data[4], data[5], data[6], data[7],
        data[8], data[9], data[10], data[11]
    };
    _address64 = XBeeAddress(address64);

    _address16[0] = data[12];
    _address16[1] = data[13];
    _options = data[14];

    int rfDataLength = _length[1] - 12;
    unsigned char rfData[rfDataLength];
    for (int i = 0; i < rfDataLength; i++) {
        rfData[i] = data[15 + i];
    }
    _rfData = rfData;
    _checksum = data[_length[1] + 3];
}

XBeeAddress Response::getAddress()
{
    return _address64;
}

unsigned char* Response::getRfData()
{
    return _rfData;
}

unsigned char BaseResponse::getMsb() { return msb; }
unsigned char BaseResponse::getLsb() { return lsb; }
unsigned char BaseResponse::getFrameType() { return frameType; }
unsigned char BaseResponse::getFrameId() { return frameId; }
unsigned char BaseResponse::getChecksum() { return checksum; }
void BaseResponse::setMsb(unsigned char msb) { BaseResponse::msb = msb; }
void BaseResponse::setChecksum(unsigned char checksum) { BaseResponse::checksum = checksum; }
void BaseResponse::setFrameId(unsigned char frameId) { BaseResponse::frameId = frameId; }
void BaseResponse::setFrameType(unsigned char frameType) { BaseResponse::frameType = frameType; }
void BaseResponse::setLsb(unsigned char lsb) { BaseResponse::lsb = lsb; }

AtCommandResponse::AtCommandResponse() {
}

bool AtCommandResponse::isSuccess() {
    return commandStatus == 0x00;
}

RemoteAtCommandResponse::RemoteAtCommandResponse() {
}

bool RemoteAtCommandResponse::isSuccess() {
    return commandStatus == 0x00;
}

XBeeClient::XBeeClient()
{
    _apiMode = 2;
}

void XBeeClient::setSerial(int speed)
{
    Serial.begin(speed);
}

int XBeeClient::available()
{
    return Serial.available();
}

void XBeeClient::write(unsigned char data)
{
    if (_apiMode == 2) {
        if (data == START_DELIMITER ||
            data == ESCAPE_CHARACTER ||
            data == XON_CHARACTER ||
            data == XOFF_CHARACTER) {
            Serial.write(ESCAPE_CHARACTER);
            data = data ^ 0x20;
        }
    }
    Serial.write(data);
}

void XBeeClient::send(AtCommandRequest request) {
    Serial.write(START_DELIMITER);
    write(LENGTH_MSB);
    write(request.getLsb());
    for(unsigned char i = 0; i < request.getLsb(); i++) {
        write(request.getFrameData(i));
    }
    write(request.checkSum());
}

void XBeeClient::send(RemoteATCommandRequest request)
{
    Serial.write(START_DELIMITER);
    write(LENGTH_MSB);
    write(request.getLsb());
    for(unsigned char i = 0; i < request.getLsb(); i++) {
        write(request.getFrameData(i));
    }
    write(request.checkSum());
}

void XBeeClient::send(Request request, XBeeAddress address)
{
    Serial.write(START_DELIMITER);

    write(LENGTH_MSB);
    write(calcLengthLSB(request));

    write(TRANSMIT_REQUEST);
    write(FRAME_ID);

    // 64-bit DestinationAddress
    for (int i = 0; i < 8; i++) {
        write(address.getAddress(i));
    }
    for (int i = 0; i < 2; i++) {
        write(RESERVED[i]);
    }
    write(BROADCAST_REDIUS);
    write(TRANSMIT_OPTION);

    for (int i = 0; i < request.getRfDataSize(); i++) {
      write(request.getRfData(i));
    }

    unsigned char checksum = calcChecksum(request, address);
    write(checksum);
}

Response XBeeClient::getResponse()
{
    Response response = Response();

    unsigned char packet = Serial.read();
    if (packet == START_DELIMITER) {
        unsigned char lengthMsb = readPacket();
        unsigned char lengthLsb = readPacket();

        int responseLength = lengthLsb + 3;
        unsigned char packets[responseLength];
        packets[0] = START_DELIMITER;
        packets[1] = lengthMsb;
        packets[2] = lengthLsb;
        for (int i = 3; i < responseLength; i++) {
            packets[i] = readPacket();
        }
        response.setData(packets);
    }

    return response;
}

void XBeeClient::readResponse(AtCommandResponse* response) {
    unsigned char packet = Serial.read();
    if (packet == START_DELIMITER) {
        response->setMsb(readPacket());
        response->setLsb(readPacket());
        response->setFrameType(readPacket());
        response->setFrameId(readPacket());

        response->command[0] = readPacket();
        response->command[1] = readPacket();
        response->commandStatus = readPacket();

        unsigned char commandDataLength = (unsigned char) (response->getLsb() - 5);
        if (commandDataLength > 0) {
            for (int i = 0; i < commandDataLength; i++) {
                response->commandData[i] = readPacket();
            }
        }
        response->setChecksum(readPacket());
    }
}

void XBeeClient::readResponse(RemoteAtCommandResponse* response) {
    unsigned char packet = Serial.read();
    if (packet == START_DELIMITER) {
        response->setMsb(readPacket());
        response->setLsb(readPacket());
        response->setFrameType(readPacket());
        response->setFrameId(readPacket());

        unsigned char address[8];
        for (int i = 0; i < 8; i++) {
           address[i]  = readPacket();
        }
        response->xbeeAddress = XBeeAddress(address);
        response->reserved[0] = readPacket();
        response->reserved[1] = readPacket();
        response->command[0] = readPacket();
        response->command[1] = readPacket();
        response->commandStatus = readPacket();

        unsigned char commandDataLength = (unsigned char) (response->getLsb() - 15);
        if (commandDataLength > 0) {
            for (int i = 0; i < commandDataLength; i++) {
                response->commandData[i] = readPacket();
            }
        }
        response->setChecksum(readPacket());
    }
}

unsigned char XBeeClient::readPacket()
{
  unsigned char packet = Serial.read();

  if (_apiMode == 2) {
      if (packet == ESCAPE_CHARACTER) {
          packet = Serial.read();
          packet = packet ^ 0x20;
      }
  }

  return packet;
}

unsigned char XBeeClient::calcLengthLSB(Request request)
{
    return 0x0E + request.getRfDataSize();
}

unsigned char XBeeClient::calcChecksum(Request request, XBeeAddress address)
{
  unsigned char sum = TRANSMIT_REQUEST + FRAME_ID;
  for (int i = 0; i < 8; i++) {
    sum += address.getAddress(i);
  }
  sum += RESERVED[0] + RESERVED[1];
  sum += BROADCAST_REDIUS;
  sum += TRANSMIT_OPTION;
  for (int i = 0; i < request.getRfDataSize(); i++) {
      sum += request.getRfData(i);
  }
  return 0xFF - sum;
}
