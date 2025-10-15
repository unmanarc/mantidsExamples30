#pragma once


#include <Mantids30/DB_SQLite3/sqlconnector_sqlite3.h>
#include <boost/property_tree/ptree_fwd.hpp>
#include <Mantids30/Threads/lock_shared.h>
#include <Mantids30/Config_Builder/program_logs.h>
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

    Mantids30::Threads::Sync::Mutex_Shared dbShrLock;

    SQLConnector_SQLite3 *dbConnector = nullptr;
};

extern AppContext g_ctx;
