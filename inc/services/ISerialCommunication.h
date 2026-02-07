#ifndef I_SERIAL_COMMUNICATION_H
#define I_SERIAL_COMMUNICATION_H

// System includes
#include <string>
#include <functional>

namespace media_player 
{
namespace services 
{

// Callback types
using SerialDataCallback = std::function<void(const std::string& data)>;
using SerialErrorCallback = std::function<void(const std::string& error)>;

class ISerialCommunication 
{
public:
    virtual ~ISerialCommunication() = default;
    
    // Connection
    virtual bool open(const std::string& portName, int baudRate) = 0;
    virtual void close() = 0;
    virtual bool isOpen() const = 0;
    
    // Data transfer
    virtual bool sendData(const std::string& data) = 0;
    virtual std::string readData() = 0;
    
    // Callbacks
    virtual void setDataCallback(SerialDataCallback callback) = 0;
    virtual void setErrorCallback(SerialErrorCallback callback) = 0;
};

} // namespace services
} // namespace media_player

#endif // I_SERIAL_COMMUNICATION_H