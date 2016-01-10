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
  _frameType = REMOTE_AT_COMMAND_REQUEST;
  _frameId = 0x01;
  _lengthLSB = 0x10;
  remoteCommandOptions = 0x02;
}

unsigned char RemoteATCommandRequest::getLengthLSB()
{
  return _lengthLSB;
}

unsigned char* RemoteATCommandRequest::getFrameData()
{
  unsigned char frameData[16];
  frameData[0] = _frameType;
  frameData[1] = _frameId;

  for(int i = 0; i < 8; i++) {
    frameData[i+2] = xbeeAddress.getAddress(i);
  }

  // Reserved
  frameData[10] = 0xFF;
  frameData[11] = 0xFE;

  frameData[12] = remoteCommandOptions;
  frameData[13] = atCommand[0];
  frameData[14] = atCommand[1];
  frameData[15] = commandParameter;

  return frameData;
}

unsigned char RemoteATCommandRequest::getChecksum()
{
  unsigned char sum = 0x00;
  unsigned char* frameData = getFrameData();
  for(int i = 0; i < _lengthLSB; i++) {
    sum = sum + frameData[i];
  }
  return 0xFF - sum;
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

void XBeeClient::send(RemoteATCommandRequest request)
{
    Serial.write(START_DELIMITER);
    write(0x00);
    write(request.getLengthLSB());

    unsigned char* frameData = request.getFrameData();
    for(int i = 0; i < request.getLengthLSB(); i++) {
      write(frameData[i]);
    }
    write(request.getChecksum());
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

