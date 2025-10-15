#include "dbinit.h"

#include "definitions/context.h"
#include "definitions/database.h"
#include "config.h"
#include <filesystem>

#include <sys/stat.h>

using namespace Mantids30;

bool initTables()
{
    bool success = true;
    for (const auto &sql : getSQLCreateStatements())
    {
        if (!g_ctx.dbConnector->execute(sql.data()))
        {
            APP_LOG->log0(__func__, Logs::LEVEL_CRITICAL, "Failed to execute SQL: '%s'", std::string(sql).c_str());
            success = false;
            break;
        }
    }

    return success;
}

bool initDatabase()
{
    std::string dbDirectory;
    try
    {
        dbDirectory = g_ctx.config.get<std::string>("DB.Directory", "var/lib/" PROJECT_NAME);
        // Create directory if it doesn't exist
        std::filesystem::path dirPath(dbDirectory);
        if (!std::filesystem::exists(dirPath))
        {
            if (std::filesystem::create_directories(dirPath))
            {
                APP_LOG->log0("DB", Logs::LEVEL_INFO, "Database directory created: %s", dbDirectory.c_str());
            }
            else
            {
                APP_LOG->log0("DB", Logs::LEVEL_ERR, "Failed to create database directory: %s", dbDirectory.c_str());
                return false;
            }
        }
        else
        {
            APP_LOG->log0("DB", Logs::LEVEL_INFO, "Database directory already exists: %s", dbDirectory.c_str());
        }
    }
    catch (const std::exception &e)
    {
        APP_LOG->log0("DB", Logs::LEVEL_ERR, "Error initializing database: %s", e.what());

        return false;
    }

    g_ctx.dbConnector = new SQLConnector_SQLite3();

    g_ctx.dbConnector->setThrowCPPErrorOnQueryFailure(g_ctx.config.get<bool>("DB.TerminateOnSQLError", false));

    if (!g_ctx.dbConnector->connectInMemory())
    {
        APP_LOG->log0(__func__, Logs::LEVEL_CRITICAL, "Error, Failed to create in-memory SQLite3 database");
        delete g_ctx.dbConnector;
        return false;
    }

    auto createFileIfNotExists = [](const std::string &path) -> bool
    {
        struct stat buffer;
        if (stat(path.c_str(), &buffer) == 0)
            return true;
        std::ofstream file(path);
        if (!file.is_open())
        {
            APP_LOG->log0(__func__, Logs::LEVEL_CRITICAL, "Failed to create database file: '%s'", path.c_str());
            return false;
        }
        file.close();
#ifdef WIN32
        _chmod(path.c_str(), _S_IREAD | _S_IWRITE);
#else
        chmod(path.c_str(), 0600);
#endif
        return true;
    };

    for (const auto & i : databaseDefinitions())
    {
        std::string messageBoardDBPath = dbDirectory + "/" + i.second;

        if (!createFileIfNotExists(messageBoardDBPath))
        {
            delete g_ctx.dbConnector;
            return false;
        }

        if (!g_ctx.dbConnector->attach(messageBoardDBPath, i.first))
        {
            APP_LOG->log0(__func__, Logs::LEVEL_CRITICAL, "Error, Failed to attach SQLite3 database file: '%s'", messageBoardDBPath.c_str());
            delete g_ctx.dbConnector;
            return false;
        }
        else
        {
            APP_LOG->log0(__func__, Logs::LEVEL_INFO, "Opened SQLite3 database file: '%s' into '%s'", messageBoardDBPath.c_str(),i.first.c_str());
        }
    }

    return initTables();
}
