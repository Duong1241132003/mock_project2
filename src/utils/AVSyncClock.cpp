// Project includes
#include "utils/AVSyncClock.h"

namespace media_player 
{
namespace utils 
{

AVSyncClock::AVSyncClock()
    : m_isRunning(false)
    , m_isPaused(false)
    , m_pausedDuration(0.0)
    , m_masterTimeOffset(0.0)
{
}

void AVSyncClock::start() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_startTime = std::chrono::high_resolution_clock::now();
    m_isRunning = true;
    m_isPaused = false;
    m_pausedDuration = 0.0;
}

void AVSyncClock::pause() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isRunning || m_isPaused) 
    {
        return;
    }
    
    m_pauseTime = std::chrono::high_resolution_clock::now();
    m_isPaused = true;
}

void AVSyncClock::resume() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isRunning || !m_isPaused) 
    {
        return;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto pauseDuration = std::chrono::duration_cast<std::chrono::microseconds>(
        now - m_pauseTime
    ).count() / 1000000.0;
    
    m_pausedDuration += pauseDuration;
    m_isPaused = false;
}

void AVSyncClock::reset() 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    m_isRunning = false;
    m_isPaused = false;
    m_pausedDuration = 0.0;
    m_masterTimeOffset = 0.0;
}

double AVSyncClock::getCurrentTime() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    if (!m_isRunning) 
    {
        return 0.0;
    }
    
    if (m_isPaused) 
    {
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
            m_pauseTime - m_startTime
        ).count() / 1000000.0;
        
        return elapsed - m_pausedDuration + m_masterTimeOffset;
    }
    
    auto now = std::chrono::high_resolution_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
        now - m_startTime
    ).count() / 1000000.0;
    
    return elapsed - m_pausedDuration + m_masterTimeOffset;
}

double AVSyncClock::getElapsedTime() const 
{
    return getCurrentTime();
}

void AVSyncClock::setMasterTime(double timeSeconds) 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    
    double currentTime = getCurrentTime();
    m_masterTimeOffset = timeSeconds - currentTime;
}

double AVSyncClock::getTimeDrift() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_masterTimeOffset;
}

} // namespace utils
} // namespace media_player