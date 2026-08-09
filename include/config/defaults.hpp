#pragma once

#include "version.hpp"

namespace config
{

constexpr auto g_ServerName = "Run Uram Server - runuram";

constexpr auto g_ServerVersion = PROJECT_VERSION;

constexpr auto g_DefaultServerDir = "/opt/runuram/";
constexpr auto g_DefaultTmpDir = "/tmp/runuram/";
constexpr auto g_DefaultLoggerDir = "/var/log/runuram/";
constexpr auto g_DefaultSSLDir  = "/opt/runuram/ssl";
constexpr auto g_DefaultConfigDir  = "/etc/runuram/";

constexpr auto g_SSLDefaultRunStatus = false;
constexpr auto g_DefaultSSLCertFile = "/opt/runuram/ssl/cert.pem";
constexpr auto g_DefaultSSLKeyFile =  "/opt/runuram/ssl/key.pem";

constexpr auto g_DefaultConfigFile =  "/etc/runuram/runuram.cfg";

constexpr int g_DefaultThreads = 4;

constexpr auto g_DefaultServerAddress = "0.0.0.0";
constexpr unsigned short g_DefaultHTTPSPort = 8080;
constexpr unsigned short g_DefaultHTTPPort  = 8081;
constexpr unsigned short g_DefaultWebSocketPort = 9090;

constexpr auto g_LoggerDefaultLevel = "info";
constexpr auto g_LoggerDefaultPattern = "[%Y-%m-%d %H:%M:%S.%f] [%l] %v";
constexpr size_t g_LoggerDefaultMaxSize = 5 * 1024 * 1024; // 5 MB
constexpr size_t g_LoggerDefaultMaxFiles = 3;
constexpr int g_LoggerDefaultFlushInterval = 3; // seconds

} // namespace config
