// Project includes
#include "core/Application.h"
#include "utils/Logger.h"

// System includes
#include <iostream>
#include <exception>

int main(int argc, char* argv[]) 
{
    try 
    {
        // Initialize logger
        media_player::utils::Logger::getInstance().setLogFile("./logs/app.log");
        media_player::utils::Logger::getInstance().setLogLevel(media_player::utils::LogLevel::DEBUG);
        
        LOG_INFO("========================================");
        LOG_INFO("Media Player Application Starting...");
        LOG_INFO("========================================");
        
        // Create and run application
        media_player::core::Application& app = media_player::core::Application::getInstance();
        
        if (!app.initialize()) 
        {
            LOG_FATAL("Failed to initialize application");
            return 1;
        }
        
        int exitCode = app.run();
        
        LOG_INFO("========================================");
        LOG_INFO("Media Player Application Exiting...");
        LOG_INFO("Exit Code: " + std::to_string(exitCode));
        LOG_INFO("========================================");
        
        return exitCode;
    }
    catch (const std::exception& e) 
    {
        std::cerr << "Fatal exception: " << e.what() << std::endl;
        LOG_FATAL("Fatal exception: " + std::string(e.what()));
        return 1;
    }
    catch (...) 
    {
        std::cerr << "Unknown fatal exception" << std::endl;
        LOG_FATAL("Unknown fatal exception");
        return 1;
    }
}