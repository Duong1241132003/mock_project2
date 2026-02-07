#ifndef OBSERVER_H
#define OBSERVER_H

// System includes
#include <vector>
#include <memory>
#include <algorithm>

namespace media_player 
{
namespace utils 
{

// Generic Observer pattern implementation
template<typename T>
class IObserver 
{
public:
    virtual ~IObserver() = default;
    virtual void onNotify(const T& data) = 0;
};

template<typename T>
class Subject 
{
public:
    void attach(std::shared_ptr<IObserver<T>> observer) 
    {
        m_observers.push_back(observer);
    }
    
    void detach(std::shared_ptr<IObserver<T>> observer) 
    {
        m_observers.erase(
            std::remove(m_observers.begin(), m_observers.end(), observer),
            m_observers.end()
        );
    }
    
    void notify(const T& data) 
    {
        for (auto& observer : m_observers) 
        {
            if (auto obs = observer.lock()) 
            {
                obs->onNotify(data);
            }
        }
    }
    
private:
    std::vector<std::weak_ptr<IObserver<T>>> m_observers;
};

} // namespace utils
} // namespace media_player

#endif // OBSERVER_H