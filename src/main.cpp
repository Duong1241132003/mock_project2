// Project includes
#include "core/Application.h"

// System includes
#include <exception>

int main(int argc, char* argv[]) 
{
    try 
    {
        // Create and run application
        media_player::core::Application& app = media_player::core::Application::getInstance();
        
        if (!app.initialize()) 
        {
            return 1;
        }
        
        int exitCode = app.run();
        
        return exitCode;
    }
    catch (const std::exception& e) 
    {
        return 1;
    }
    catch (...) 
    {
        return 1;
    }
}
