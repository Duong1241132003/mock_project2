// Project includes
#include "models/PlaybackStateModel.h"

// System includes
#include <sstream>
#include <iomanip>

namespace media_player 
{
namespace models 
{

// setVolume moved to header inline

std::string PlaybackStateModel::getFormattedPosition() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int hours = m_currentPositionSeconds / 3600;
    int minutes = (m_currentPositionSeconds % 3600) / 60;
    int seconds = m_currentPositionSeconds % 60;
    
    std::ostringstream oss;
    
    if (hours > 0) 
    {
        oss << std::setfill('0') << std::setw(2) << hours << ":";
    }
    
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    
    return oss.str();
}

std::string PlaybackStateModel::getFormattedDuration() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    int hours = m_totalDurationSeconds / 3600;
    int minutes = (m_totalDurationSeconds % 3600) / 60;
    int seconds = m_totalDurationSeconds % 60;
    
    std::ostringstream oss;
    
    if (hours > 0) 
    {
        oss << std::setfill('0') << std::setw(2) << hours << ":";
    }
    
    oss << std::setfill('0') << std::setw(2) << minutes << ":"
        << std::setfill('0') << std::setw(2) << seconds;
    
    return oss.str();
}

float PlaybackStateModel::getProgressPercentage() const 
{
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_totalDurationSeconds <= 0) 
    {
        return 0.0f;
    }
    
    return (static_cast<float>(m_currentPositionSeconds) / 
            static_cast<float>(m_totalDurationSeconds)) * 100.0f;
}

} // namespace models
} // namespace media_player