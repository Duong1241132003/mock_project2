#ifndef MOCK_SERIAL_COMMUNICATION_H
#define MOCK_SERIAL_COMMUNICATION_H

#include <gmock/gmock.h>
#include "services/ISerialCommunication.h"

namespace media_player {
namespace test {

class MockSerialCommunication : public services::ISerialCommunication {
public:
    MOCK_METHOD(bool, open, (const std::string& portName, int baudRate), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, isOpen, (), (const, override));
    
    MOCK_METHOD(bool, sendData, (const std::string& data), (override));
    MOCK_METHOD(std::string, readData, (), (override));
    
    MOCK_METHOD(void, setDataCallback, (services::SerialDataCallback callback), (override));
    MOCK_METHOD(void, setErrorCallback, (services::SerialErrorCallback callback), (override));
};

} // namespace test
} // namespace media_player

#endif // MOCK_SERIAL_COMMUNICATION_H
