// Project includes
#include "views/MainScreen.h"

namespace media_player 
{
namespace views 
{

MainScreen::MainScreen()
    : m_isVisible(false)
{
}

MainScreen::~MainScreen() 
{
}

void MainScreen::show() 
{
    m_isVisible = true;
}

void MainScreen::hide() 
{
    m_isVisible = false;
}

void MainScreen::update() 
{
    if (!m_isVisible) 
    {
        return;
    }
    
    // Main screen update logic
}

bool MainScreen::isVisible() const 
{
    return m_isVisible;
}

} // namespace views
} // namespace media_player
