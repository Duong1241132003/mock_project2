#ifndef AV_SYNC_CLOCK_H
#define AV_SYNC_CLOCK_H

// System includes
#include <chrono>
#include <atomic>
#include <mutex>

namespace media_player 
{
namespace utils 
{

class AVSyncClock 
{
public:
    AVSyncClock();
    ~AVSyncClock() = default;
    
    // Clock control
    void start();
    void pause();
    void resume();
    void reset();
    
    // Time queries
    double getCurrentTime() const;
    double getElapsedTime() const;
    
    // Synchronization
    void setMasterTime(double timeSeconds);
    double getTimeDrift() const;
    
    // State
    bool isRunning() const 
    { 
        return m_isRunning; 
    }
    
private:
    std::chrono::high_resolution_clock::time_point m_startTime;
    std::chrono::high_resolution_clock::time_point m_pauseTime;
    
    std::atomic<bool> m_isRunning;
    std::atomic<bool> m_isPaused;
    
    double m_pausedDuration;
    double m_masterTimeOffset;
    
    mutable std::mutex m_mutex;
};

} // namespace utils
} // namespace media_player

#endif // AV_SYNC_CLOCK_H