#pragma once


#include <Mantids30/DB_SQLite3/sqlconnector_sqlite3.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include <shared_mutex>
#include <Mantids30/Config_Logs/config_builder.h>
#include <memory>
#include <string>
#include <ctime>

using namespace Mantids30::Program;
using namespace Mantids30::Database;

#define APP_LOG g_ctx.appLog
#define RPC_LOG g_ctx.rpcLog

struct AppContext
{
    boost::property_tree::ptree config;

    std::string configDir;

    time_t startTime;

    std::shared_ptr<Logs::AppLog> appLog;
    std::shared_ptr<Logs::RPCLog> rpcLog;

    std::shared_mutex dbShrLock;

    SQLConnector_SQLite3 *dbConnector = nullptr;
};

extern AppContext g_ctx;
