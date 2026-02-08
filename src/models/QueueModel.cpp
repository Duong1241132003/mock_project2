// Project includes
#include "models/QueueModel.h"

// System includes
#include <algorithm>
#include <random>
#include <fstream>
#include <chrono>

namespace media_player 
{
namespace models 
{

void QueueModel::addToEnd(const MediaFileModel& media) 
{
    m_items.push_back(media);
    if (m_shuffleEnabled) 
    {
        updateShuffleOrder();
    }
}

void QueueModel::addNext(const MediaFileModel& media) 
{
    if (m_items.empty()) 
    {
        m_items.push_back(media);
    }
    else 
    {
        m_items.insert(m_items.begin() + m_currentIndex + 1, media);
    }
    if (m_shuffleEnabled) 
    {
        updateShuffleOrder();
    }
}

void QueueModel::addAt(const MediaFileModel& media, size_t position) 
{
    if (position >= m_items.size()) 
    {
        m_items.push_back(media);
    }
    else 
    {
        m_items.insert(m_items.begin() + position, media);
    }
    if (m_shuffleEnabled) 
    {
        updateShuffleOrder();
    }
}

bool QueueModel::removeAt(size_t index) 
{
    if (index >= m_items.size()) 
    {
        return false;
    }
    
    m_items.erase(m_items.begin() + index);
    
    if (m_currentIndex >= m_items.size() && !m_items.empty()) 
    {
        m_currentIndex = m_items.size() - 1;
    }
    
    if (m_shuffleEnabled) 
    {
        updateShuffleOrder();
    }
    
    return true;
}

bool QueueModel::removeByPath(const std::string& filePath) 
{
    auto it = std::find_if(m_items.begin(), m_items.end(),
        [&filePath](const MediaFileModel& item) {
            return item.getFilePath() == filePath;
        });
    
    if (it != m_items.end()) 
    {
        size_t index = std::distance(m_items.begin(), it);
        return removeAt(index);
    }
    
    return false;
}

void QueueModel::clear() 
{
    m_items.clear();
    m_shuffleOrder.clear();
    m_currentIndex = 0;
}

std::optional<MediaFileModel> QueueModel::getCurrentItem() const 
{
    if (m_items.empty() || m_currentIndex >= m_items.size()) 
    {
        return std::nullopt;
    }
    
    size_t actualIndex = getActualIndex(m_currentIndex);
    return m_items[actualIndex];
}

std::optional<MediaFileModel> QueueModel::getNextItem() const 
{
    if (m_items.empty()) 
    {
        return std::nullopt;
    }
    
    size_t nextIndex = m_currentIndex + 1;
    
    if (nextIndex >= m_items.size()) 
    {
        if (m_repeatMode == RepeatMode::LoopAll) 
        {
            nextIndex = 0;
        }
        else 
        {
            return std::nullopt;
        }
    }
    
    size_t actualIndex = getActualIndex(nextIndex);
    return m_items[actualIndex];
}

std::optional<MediaFileModel> QueueModel::getPreviousItem() const 
{
    if (m_items.empty()) 
    {
        return std::nullopt;
    }
    
    if (m_currentIndex == 0) 
    {
        if (m_repeatMode == RepeatMode::LoopAll) 
        {
            size_t actualIndex = getActualIndex(m_items.size() - 1);
            return m_items[actualIndex];
        }
        return std::nullopt;
    }
    
    size_t actualIndex = getActualIndex(m_currentIndex - 1);
    return m_items[actualIndex];
}

std::optional<MediaFileModel> QueueModel::getItemAt(size_t index) const 
{
    if (index >= m_items.size()) 
    {
        return std::nullopt;
    }
    return m_items[index];
}

std::vector<MediaFileModel> QueueModel::getItemsInPlaybackOrder() const
{
    if (m_items.empty()) return {};
    if (!m_shuffleEnabled) return m_items;
    std::vector<MediaFileModel> out;
    out.reserve(m_items.size());
    for (size_t i = 0; i < m_items.size(); ++i)
        out.push_back(m_items[getActualIndex(i)]);
    return out;
}

bool QueueModel::moveToNext() 
{
    if (m_items.empty()) 
    {
        return false;
    }
    
    m_currentIndex++;
    
    if (m_currentIndex >= m_items.size()) 
    {
        if (m_repeatMode == RepeatMode::LoopAll)
        {
            m_currentIndex = 0;
            return true;
        }
        m_currentIndex = m_items.size() - 1;
        return false;
    }
    
    return true;
}

bool QueueModel::moveToPrevious() 
{
    if (m_items.empty() || m_currentIndex == 0) 
    {
        if (m_repeatMode == RepeatMode::LoopAll && !m_items.empty()) 
        {
            m_currentIndex = m_items.size() - 1;
            return true;
        }
        return false;
    }
    
    m_currentIndex--;
    return true;
}

bool QueueModel::jumpTo(size_t index) 
{
    if (index >= m_items.size()) 
    {
        return false;
    }
    
    m_currentIndex = index;
    return true;
}

bool QueueModel::hasNext() const 
{
    if (m_items.empty()) 
    {
        return false;
    }
    
    if (m_repeatMode == RepeatMode::LoopAll) 
    {
        return true;
    }
    
    return m_currentIndex < m_items.size() - 1;
}

bool QueueModel::hasPrevious() const 
{
    if (m_items.empty()) 
    {
        return false;
    }
    
    if (m_repeatMode == RepeatMode::LoopAll) 
    {
        return true;
    }
    
    return m_currentIndex > 0;
}

bool QueueModel::moveItem(size_t fromIndex, size_t toIndex) 
{
    if (fromIndex >= m_items.size() || toIndex >= m_items.size()) 
    {
        return false;
    }
    
    MediaFileModel item = m_items[fromIndex];
    m_items.erase(m_items.begin() + fromIndex);
    m_items.insert(m_items.begin() + toIndex, item);
    
    // Adjust current index if needed
    if (m_currentIndex == fromIndex) 
    {
        m_currentIndex = toIndex;
    }
    else if (fromIndex < m_currentIndex && toIndex >= m_currentIndex) 
    {
        m_currentIndex--;
    }
    else if (fromIndex > m_currentIndex && toIndex <= m_currentIndex) 
    {
        m_currentIndex++;
    }
    
    return true;
}

void QueueModel::setShuffleMode(bool enabled) 
{
    m_shuffleEnabled = enabled;
    if (enabled) 
    {
        updateShuffleOrder();
    }
}

void QueueModel::setRepeatMode(RepeatMode mode) 
{
    m_repeatMode = mode;
    const char* name = (mode == RepeatMode::None) ? "None" : 
                       (mode == RepeatMode::LoopOne) ? "LoopOne" : "LoopAll";
}

void QueueModel::updateShuffleOrder() 
{
    m_shuffleOrder.clear();
    if (m_items.empty()) return;
    m_shuffleOrder.reserve(m_items.size());
    
    for (size_t i = 0; i < m_items.size(); i++) 
    {
        m_shuffleOrder.push_back(i);
    }
    
    // Fisher-Yates shuffle
    std::random_device rd;
    std::mt19937 gen(rd());
    
    for (size_t i = m_shuffleOrder.size() - 1; i > 0; i--) 
    {
        std::uniform_int_distribution<size_t> dist(0, i);
        size_t j = dist(gen);
        std::swap(m_shuffleOrder[i], m_shuffleOrder[j]);
    }
}

size_t QueueModel::getActualIndex(size_t logicalIndex) const 
{
    if (m_shuffleEnabled && logicalIndex < m_shuffleOrder.size()) 
    {
        return m_shuffleOrder[logicalIndex];
    }
    return logicalIndex;
}

} // namespace models
} // namespace media_player
