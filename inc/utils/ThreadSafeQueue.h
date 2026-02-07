#ifndef THREAD_SAFE_QUEUE_H
#define THREAD_SAFE_QUEUE_H

// System includes
#include <queue>
#include <mutex>
#include <condition_variable>
#include <optional>

namespace media_player 
{
namespace utils 
{

template<typename T>
class ThreadSafeQueue 
{
public:
    ThreadSafeQueue() : m_stopped(false) {}
    ~ThreadSafeQueue() = default;
    
    // Delete copy operations
    ThreadSafeQueue(const ThreadSafeQueue&) = delete;
    ThreadSafeQueue& operator=(const ThreadSafeQueue&) = delete;
    
    void push(const T& item) 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(item);
        m_condition.notify_one();
    }
    
    void push(T&& item) 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_queue.push(std::move(item));
        m_condition.notify_one();
    }
    
    std::optional<T> pop() 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_queue.empty()) 
        {
            return std::nullopt;
        }
        T item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }
    
    // Chờ và lấy item từ queue, trả về nullopt nếu queue bị stop
    std::optional<T> waitAndPop() 
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_condition.wait(lock, [this] 
        { 
            return !m_queue.empty() || m_stopped; 
        });
        
        // Kiểm tra nếu bị stop và queue rỗng
        if (m_stopped && m_queue.empty())
        {
            return std::nullopt;
        }
        
        T item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }
    
    // Chờ với timeout, trả về nullopt nếu hết thời gian hoặc bị stop
    std::optional<T> waitAndPopTimeout(int timeoutMs) 
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        bool result = m_condition.wait_for(lock, 
            std::chrono::milliseconds(timeoutMs),
            [this] 
            { 
                return !m_queue.empty() || m_stopped; 
            });
        
        // Kiểm tra timeout hoặc stop
        if (!result || m_stopped && m_queue.empty())
        {
            return std::nullopt;
        }
        
        if (m_queue.empty())
        {
            return std::nullopt;
        }
        
        T item = std::move(m_queue.front());
        m_queue.pop();
        return item;
    }
    
    bool empty() const 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.empty();
    }
    
    size_t size() const 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_queue.size();
    }
    
    void clear() 
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::queue<T> empty;
        std::swap(m_queue, empty);
    }
    
    // Dừng tất cả các thread đang chờ waitAndPop
    void stop()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = true;
        m_condition.notify_all();
    }
    
    // Reset trạng thái stop để có thể sử dụng lại queue
    void reset()
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_stopped = false;
    }
    
    // Kiểm tra trạng thái stop
    bool isStopped() const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        return m_stopped;
    }
    
private:
    mutable std::mutex m_mutex;
    std::queue<T> m_queue;
    std::condition_variable m_condition;
    bool m_stopped;  // Flag để hủy các thread đang chờ
};

} // namespace utils
} // namespace media_player

#endif // THREAD_SAFE_QUEUE_H