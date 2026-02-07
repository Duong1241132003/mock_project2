#ifndef MAIN_SCREEN_H
#define MAIN_SCREEN_H

// System includes
#include <memory>

// Project includes
#include "IView.h"

namespace media_player 
{
namespace views 
{

class MainScreen : public IView 
{
public:
    MainScreen();
    ~MainScreen();
    
    // IView interface
    void show() override;
    void hide() override;
    void update() override;
    bool isVisible() const override;
    
private:
    bool m_isVisible;
};

} // namespace views
} // namespace media_player

#endif // MAIN_SCREEN_H