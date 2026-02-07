#ifndef I_REPOSITORY_H
#define I_REPOSITORY_H

// System includes
#include <vector>
#include <optional>
#include <string>

namespace media_player 
{
namespace repositories 
{

template<typename T>
class IRepository 
{
public:
    virtual ~IRepository() = default;
    
    // CRUD operations
    virtual bool save(const T& item) = 0;
    virtual std::optional<T> findById(const std::string& id) = 0;
    virtual std::vector<T> findAll() = 0;
    virtual bool update(const T& item) = 0;
    virtual bool remove(const std::string& id) = 0;
    virtual bool exists(const std::string& id) = 0;
    
    // Bulk operations
    virtual bool saveAll(const std::vector<T>& items) = 0;
    virtual void clear() = 0;
    
    // Statistics
    virtual size_t count() const = 0;
};

} // namespace repositories
} // namespace media_player

#endif // I_REPOSITORY_H