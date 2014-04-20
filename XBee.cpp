#include "Arduino.h"
#include "XBee.h"

const unsigned char START_DELIMITER = 0x7E;
const unsigned char LENGTH_MSB = 0x00;
const unsigned char LENGTH_LSB = 0x11;
const unsigned char FRAME_TYPE = 0x10;
const unsigned char FRAME_ID = 0x00;
const unsigned char RESERVED[2] = {0xFF, 0xFE};
const unsigned char BROADCAST_REDIUS = 0x00;
const unsigned char TRANSMIT_OPTION = 0x00;

XBeeAddress::XBeeAddress()
{
    _isEmpty = true;
}

XBeeAddress::XBeeAddress(unsigned char addr[8])
{
    for (int i = 0; i < 8; i++) {
        _address[i] = addr[i];
    }
    _isEmpty = false;
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
        if (data == 0x7E || data == 0x7D || data == 0x11 || data == 0x13) {
            Serial.write(0x7D);
            data = data ^ 0x20;
        }
    }
    Serial.write(data);
}

void XBeeClient::send(Request request, XBeeAddress address)
{
    Serial.write(START_DELIMITER);

    write(LENGTH_MSB);
    write(LENGTH_LSB);

    write(FRAME_TYPE);
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

    for (int i = 0; i < request._rfDataSize; i++) {
      write(request._rfData[i]);
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

unsigned char XBeeClient::readPacket()
{
  unsigned char packet = Serial.read();

  if (_apiMode == 2) {
      if (packet == 0x7D) {
          packet = Serial.read();
          packet = packet ^ 0x20;
      }
  }

  return packet;
}

unsigned char XBeeClient::calcChecksum(Request request, XBeeAddress address)
{
  unsigned char sum = FRAME_TYPE + FRAME_ID;
  for (int i = 0; i < 8; i++) {
    sum += address.getAddress(i);
  }
  sum += RESERVED[0] + RESERVED[1];
  sum += BROADCAST_REDIUS;
  sum += TRANSMIT_OPTION;
  for (int i = 0; i < request._rfDataSize; i++) {
      sum += request._rfData[i];
  }
  return 0xFF - sum;
}

