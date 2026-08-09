#include "config/config.hpp"
#include "logger/logger.hpp"
#include "server/server.hpp"

int main(int argc, char* argv[])
{
    try
    {
        logger::Logger::Create(config::ServerConfig::Get(argc, argv).GetLoggerSettings());
        
        server::Server app_server{config::ServerConfig::Get().Threads()};
        app_server.Run();

        return EXIT_SUCCESS;
    }
    catch (const config::OptionsExitsProgram& ex)
    {
        return EXIT_SUCCESS; // (--help, --version, dry-run)
    }
    catch (const std::exception& ex)
    {
        LOG_ERROR("Unhandled exception: {}", ex.what());
        return EXIT_FAILURE;
    }
}