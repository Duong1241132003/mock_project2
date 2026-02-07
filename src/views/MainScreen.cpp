// Project includes
#include "views/MainScreen.h"
#include "utils/Logger.h"

namespace media_player 
{
namespace views 
{

MainScreen::MainScreen()
    : m_isVisible(false)
{
    LOG_INFO("MainScreen created");
}

MainScreen::~MainScreen() 
{
    LOG_INFO("MainScreen destroyed");
}

void MainScreen::show() 
{
    m_isVisible = true;
    LOG_INFO("MainScreen shown");
}

void MainScreen::hide() 
{
    m_isVisible = false;
    LOG_INFO("MainScreen hidden");
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