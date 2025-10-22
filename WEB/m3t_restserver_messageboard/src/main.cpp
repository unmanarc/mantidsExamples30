/**
 * @file main.cpp
 * @brief Minimal RESTful API Server Demo using libMantids3
 *
 * This example demonstrates how to create a simple HTTPS RESTful API server
 * with JWT authentication support using the Mantids3 framework.
 *
 * Features:
 * - TLS/SSL encrypted connections
 * - RESTful API endpoint registration
 * - Configuration file support
 * - Structured logging (application & RPC)
 * - JWT token validation ready
 *
 */

#include <Mantids30/Config_Builder/program_configloader.h>

#include <Mantids30/Config_Builder/restful_engine.h>
#include <Mantids30/Net_Sockets/socket_tls.h>
#include <Mantids30/Program_Service/application.h>
#include <Mantids30/Protocol_APISync/apisync.h>
#include <boost/algorithm/string/case_conv.hpp>
#include "dbinit.h"
#include "definitions/accesscontrol.h"
#include <optional>

using namespace Mantids30;
using namespace Mantids30::Program;
using namespace Mantids30::Network::Servers;

// ============================================================================
// GLOBAL STATE
// ============================================================================

#include "definitions/context.h"
#include "endpoints/api.h"
#include "config.h"

AppContext g_ctx;

// ============================================================================
// SERVICE INITIALIZATION
// ============================================================================

/**
 * @brief Initialize and start the RESTful web service
 */
bool startWebService()
{
    // Load web service configuration
    auto webConfig = g_ctx.config.get_child_optional("WebService");
    if (!webConfig.has_value())
    {
        APP_LOG->log0(__func__, Logs::LEVEL_ERR, "Missing 'WebService' section in configuration");
        return false;
    }

    std::map<std::string, std::string> vars;

    const char* apiKey = getenv("x-api-key");
    if (apiKey == nullptr) {
        APP_LOG->log0(__func__, Logs::LEVEL_ERR, "Environment variable 'x-api-key' not defined");
        return false;
    }

    vars["APIKEY"] = std::string(apiKey);

    const char* appName = getenv("app");
    if (appName == nullptr) {
        APP_LOG->log0(__func__, Logs::LEVEL_ERR, "Environment variable 'app' not defined");
        return false;
    }

    vars["APP"] = std::string(appName);

    // Create RESTful engine with secure defaults
    auto *engine = Config::RESTful_Engine::createRESTfulEngine(*webConfig,
                                                               APP_LOG,
                                                               RPC_LOG,
                                                               boost::to_upper_copy(std::string(PROJECT_NAME)),
                                                               "/var/www/" PROJECT_NAME,  // Default web root
                                                               Config::REST_ENGINE_MANDATORY_SSL,
                                                               vars
                                                               );

    if (auto x = webConfig->get_child_optional("APISync"))
    {
        Network::Protocols::APISync::APISyncParameters parameters;
        parameters.loadFromInfoTree(*x);
        Network::Protocols::APISync::updateAccessControlContext( APP_LOG.get(), &parameters, appName,apiKey, appScopes(), appRoles(), appActivities()  );
    }


    if (!engine)
    {
        APP_LOG->log0(__func__, Logs::LEVEL_ERR, "Failed to create web service");
        return false;
    }

    // Configure service
    engine->config.appName = vars["APP"];
    engine->config.setSoftwareVersion(atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PROJECT_VER_PATCH), "stable");

    // Register API v1 endpoints
    engine->endpointsHandler[1] = registerAPIEndpoints();

    // Start service
    engine->startInBackground();

    APP_LOG->log0(__func__, Logs::LEVEL_INFO, "Web service listening on %s", engine->getListenerSocket()->getLastBindAddress().c_str());
    return true;
}

// ============================================================================
// APPLICATION LIFECYCLE
// ============================================================================

class DemoApplication : public Application
{
public:
    /**
     * @brief Initialize command-line arguments and metadata
     */
    void _initvars(int, char *[], Arguments::GlobalArguments *args) override
    {
        args->setInifiniteWaitAtEnd(true);
        args->softwareLicense = PROJECT_LICENSE;
        args->softwareDescription = PROJECT_DESCRIPTION;
        args->addAuthor({PROJECT_AUTHOR_NAME, PROJECT_AUTHOR_MAIL});
        args->setVersion(atoi(PROJECT_VER_MAJOR), atoi(PROJECT_VER_MINOR), atoi(PROJECT_VER_PATCH), "stable");

        // Command-line options
        args->addCommandLineOption("Service", 'c', "config-dir", "Configuration directory path", "/etc/" PROJECT_NAME, Memory::Abstract::Var::TYPE_STRING);
    }

    /**
     * @brief Load and validate configuration
     */
    bool _config(int, char *[], Arguments::GlobalArguments *args) override
    {
        // Initialize TLS
        Network::Sockets::Socket_TLS::prepareTLS();

        // Load configuration
        g_ctx.configDir = args->getCommandLineOptionValue("config-dir")->toString();

        auto initLog = Config::Logs::createInitLog();
        auto configOpt = Config::Loader::loadSecureApplicationConfig(initLog.get(), g_ctx.configDir.c_str(), "webserver.conf");

        if (!configOpt.has_value())
        {
            initLog->log0(__func__, Logs::LEVEL_ERR, "ERROR: Failed to load configuration from %s/webserver.conf", g_ctx.configDir.c_str());
            return false;
        }

        g_ctx.config = *configOpt;

        // Initialize logging as defined in the configuration
        APP_LOG = Config::Logs::createAppLog(g_ctx.config);
        RPC_LOG = Config::Logs::createRPCLog(g_ctx.config);

        return true;
    }

    /**
     * @brief Start the application services
     */
    int _start(int, char *[], Arguments::GlobalArguments *args) override
    {
        APP_LOG->log0(__func__, Logs::LEVEL_INFO, "%s v%s.%s.%s starting (PID: %d)", PROJECT_NAME, PROJECT_VER_MAJOR, PROJECT_VER_MINOR, PROJECT_VER_PATCH, getpid());
        APP_LOG->log0(__func__, Logs::LEVEL_INFO, "Configuration Directory: %s", g_ctx.configDir.c_str());

        g_ctx.startTime = time(nullptr);

        if (!initDatabase())
        {
            return EXIT_FAILURE;
        }

        if (!startWebService())
        {
            APP_LOG->log0(__func__, Logs::LEVEL_ERR, "Service initialization failed");
            exit(EXIT_FAILURE);
        }

        APP_LOG->log0(__func__, Logs::LEVEL_INFO, "Service ready");
        return EXIT_SUCCESS;
    }

    /**
     * @brief Clean shutdown handler
     */
    void _shutdown() override { APP_LOG->log0(__func__, Logs::LEVEL_INFO, "Shutting down..."); }
};

// ============================================================================
// ENTRY POINT
// ============================================================================

int main(int argc, char *argv[])
{
    return StartApplication(argc, argv, new DemoApplication);
}
