#ifndef SERIAL_COMMUNICATION_H
#define SERIAL_COMMUNICATION_H

// System includes
#include <string>
#include <atomic>
#include <thread>
#include <memory>
#include <mutex>

// Project includes
#include "ISerialCommunication.h"

namespace media_player 
{
namespace services 
{

class SerialCommunication : public ISerialCommunication 
{
public:
    SerialCommunication();
    ~SerialCommunication();
    
    // ISerialCommunication interface implementation
    bool open(const std::string& portName, int baudRate) override;
    void close() override;
    bool isOpen() const override;
    
    bool sendData(const std::string& data) override;
    std::string readData() override;
    
    void setDataCallback(SerialDataCallback callback) override;
    void setErrorCallback(SerialErrorCallback callback) override;
    
private:
    void readThread();
    void notifyData(const std::string& data);
    void notifyError(const std::string& error);
    
    int m_fileDescriptor;
    std::atomic<bool> m_isOpen;
    std::atomic<bool> m_shouldStop;
    std::unique_ptr<std::thread> m_readThread;
    mutable std::mutex m_mutex;
    
    SerialDataCallback m_dataCallback;
    SerialErrorCallback m_errorCallback;
};

} // namespace services
} // namespace media_player

#endif // SERIAL_COMMUNICATION_H