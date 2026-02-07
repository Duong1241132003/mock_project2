#ifndef I_VIEW_H
#define I_VIEW_H

namespace media_player 
{
namespace views 
{

class IView 
{
public:
    virtual ~IView() = default;
    
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void update() = 0;
    virtual bool isVisible() const = 0;
};

} // namespace views
} // namespace media_player

#endif // I_VIEW_H