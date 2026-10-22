#include "api.h"
#include "Mantids30/Memory/a_uint32.h"
#include "Mantids30/Protocol_HTTP/api_return.h"

#include "../definitions/context.h"
#include <json/value.h>

#include <Mantids30/Memory/a_allvars.h>

using namespace Mantids30;
using namespace Mantids30::Program;
using namespace Mantids30::Memory;
using namespace Mantids30::Network::Servers;
using namespace Mantids30::Network::Protocol;

// ============================================================================
// MESSAGEBOARD API FUNCTIONS
// ============================================================================

API::APIReturn getThreads(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::shared_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    std::string user = params.jwtToken->getSubject();

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is fetching threads");

    Abstract::UINT32 threadId;
    Abstract::STRING title, creatorUserId, createdAt, lastPostAt;
    Abstract::BOOL isPinned, isLocked;

    auto i = g_ctx.dbConnector->qSelect("SELECT `threadId`, `title`, `creatorUserId`, `createdAt`, `lastPostAt`, `isPinned`, `isLocked` "
                                                               "FROM `mboard`.`threads` WHERE `isLocked`=0 OR `isLocked`=1 ORDER BY `isPinned` DESC, `lastPostAt` DESC;",
                                                               {}, {&threadId, &title, &creatorUserId, &createdAt, &lastPostAt, &isPinned, &isLocked});

    Json::Value jsonResponse = Json::arrayValue;
    while (i && i->isSuccessful() && i->step())
    {
        Json::Value x;
        x["threadId"] = threadId.getValue();
        x["title"] = title.getValue();
        x["creatorUserId"] = creatorUserId.getValue();
        x["createdAt"] = createdAt.getValue();
        x["lastPostAt"] = lastPostAt.getValue();
        x["isPinned"] = isPinned.getValue();
        x["isLocked"] = isLocked.getValue();
        jsonResponse.append(x);
    }
    return jsonResponse;
}

API::APIReturn createThread(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    std::string title = Helpers::JSON::ASSTRING(*params.inputJSON, "title", "");
    std::string user = params.jwtToken->getSubject();

    if (title.empty())
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Thread title is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is creating thread: %s", title.c_str());

    if (!g_ctx.dbConnector->qExecuteEx("INSERT INTO `mboard`.`threads` (title, creatorUserId) VALUES (:title, :userId);", {{":title", MAKE_VAR(STRING, title)}, {":userId", MAKE_VAR(STRING, user)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    return API::APIReturn();
}

API::APIReturn getMessages(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::shared_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t threadId = Helpers::JSON::ASUINT(*params.inputJSON, "threadId", 0);
    std::string user = params.jwtToken->getSubject();

    if (threadId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Thread ID is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is fetching messages for thread %d", threadId);

    Abstract::UINT32 messageId;
    Abstract::STRING userId, content, ipAddress, userAgent, createdAt, editedAt;
    Abstract::BOOL isDeleted;

    auto i = g_ctx.dbConnector->qSelect("SELECT `messageId`, `userId`, `content`, `ipAddress`, `userAgent`, `createdAt`, `editedAt`, `isDeleted` "
                                                               "FROM `mboard`.`messages` WHERE `threadId`=:threadId AND `isDeleted`=0 ORDER BY `createdAt` ASC;",
                                                               {{":threadId", MAKE_VAR(UINT32, threadId)}}, {&messageId, &userId, &content, &ipAddress, &userAgent, &createdAt, &editedAt, &isDeleted});

    Json::Value jsonResponse = Json::arrayValue;
    while (i && i->isSuccessful() && i->step())
    {
        Json::Value x;
        x["messageId"] = messageId.getValue();
        x["userId"] = userId.getValue();
        x["content"] = content.getValue();
        x["ipAddress"] = ipAddress.getValue();
        x["userAgent"] = userAgent.getValue();
        x["createdAt"] = createdAt.getValue();
        x["editedAt"] = editedAt.getValue();
        jsonResponse.append(x);
    }
    return jsonResponse;
}

API::APIReturn postMessage(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t threadId = Helpers::JSON::ASUINT(*params.inputJSON, "threadId", 0);
    std::string content = Helpers::JSON::ASSTRING(*params.inputJSON, "content", "");
    std::string user = params.jwtToken->getSubject();

    if (threadId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Thread ID is required");
    }

    if (content.empty())
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Message content is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is posting message to thread %d", threadId);

    // Check if thread exists and is not locked
    Abstract::BOOL isLocked;
    {
        if (!g_ctx.dbConnector->qSelectSingleRow("SELECT `isLocked` FROM `mboard`.`threads` WHERE `threadId`=:threadId;", {{":threadId", MAKE_VAR(UINT32, threadId)}},
                                        {&isLocked}))
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "not_found", "Thread not found");
        }
        if (isLocked.getValue())
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_403_FORBIDDEN, "forbidden", "Thread is locked");
        }
    }

    // Insert message
    if (!g_ctx.dbConnector->qExecuteEx("INSERT INTO `mboard`.`messages` (threadId, userId, content, ipAddress, userAgent) "
                                    "VALUES (:threadId, :userId, :content, :ipAddress, :userAgent);",
                                    {{":threadId", MAKE_VAR(UINT32, threadId)},
                                     {":userId", MAKE_VAR(STRING, user)},
                                     {":content", MAKE_VAR(STRING, content)},
                                     {":ipAddress", MAKE_VAR(STRING, clientDetails.ipAddress)},
                                     {":userAgent", MAKE_VAR(STRING, clientDetails.userAgent)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    // Update thread's lastPostAt
    if (!g_ctx.dbConnector->qExecuteEx("UPDATE `mboard`.`threads` SET `lastPostAt`=CURRENT_TIMESTAMP WHERE `threadId`=:threadId;", {{":threadId", MAKE_VAR(UINT32, threadId)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed updating thread");
    }

    return API::APIReturn();
}

API::APIReturn editMessage(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t messageId = Helpers::JSON::ASUINT(*params.inputJSON, "messageId", 0);
    std::string content = Helpers::JSON::ASSTRING(*params.inputJSON, "content", "");
    std::string user = params.jwtToken->getSubject();

    if (messageId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Message ID is required");
    }

    if (content.empty())
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Message content is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is editing message %d", messageId);

    // Check if user owns the message
    Abstract::STRING messageOwner;
    {
        if (!g_ctx.dbConnector->qSelectSingleRow("SELECT `userId` FROM `mboard`.`messages` WHERE `messageId`=:messageId AND `isDeleted`=0;",
                                        {{":messageId", MAKE_VAR(UINT32, messageId)}}, {&messageOwner}))
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "not_found", "Message not found");
        }

        if (messageOwner.getValue() != user)
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_403_FORBIDDEN, "forbidden", "Not authorized to edit this message");
        }
    }

    if (!g_ctx.dbConnector->qExecuteEx("UPDATE `mboard`.`messages` SET `content`=:content, `editedAt`=CURRENT_TIMESTAMP "
                                    "WHERE `messageId`=:messageId;",
                                    {{":content", MAKE_VAR(STRING, content)}, {":messageId", MAKE_VAR(UINT32, messageId)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    return API::APIReturn();
}

API::APIReturn deleteMessage(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t messageId = Helpers::JSON::ASUINT(*params.inputJSON, "messageId", 0);
    std::string user = params.jwtToken->getSubject();

    if (messageId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Message ID is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is deleting message %d", messageId);

    // Check if user owns the message
    Abstract::STRING messageOwner;
    {
        if (!g_ctx.dbConnector->qSelectSingleRow("SELECT `userId` FROM `mboard`.`messages` WHERE `messageId`=:messageId AND `isDeleted`=0;",
                                        {{":messageId", MAKE_VAR(UINT32, messageId)}}, {&messageOwner}))
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_404_NOT_FOUND, "not_found", "Message not found");
        }

        if (messageOwner.getValue() != user && !params.jwtToken->isAdmin())
        {
            return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_403_FORBIDDEN, "forbidden", "Not authorized to delete this message");
        }
    }

    if (!g_ctx.dbConnector->qExecuteEx("UPDATE `mboard`.`messages` SET `isDeleted`=1 WHERE `messageId`=:messageId;", {{":messageId", MAKE_VAR(UINT32, messageId)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    return API::APIReturn();
}

API::APIReturn toggleThreadLock(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t threadId = Helpers::JSON::ASUINT(*params.inputJSON, "threadId", 0);
    bool lockStatus = Helpers::JSON::ASBOOL(*params.inputJSON, "isLocked", false);
    std::string user = params.jwtToken->getSubject();

    if (threadId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Thread ID is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is toggling lock for thread %d", threadId);

    if (!g_ctx.dbConnector->qExecuteEx("UPDATE `mboard`.`threads` SET `isLocked`=:isLocked WHERE `threadId`=:threadId;",
                                    {{":isLocked", MAKE_VAR(BOOL, lockStatus)}, {":threadId", MAKE_VAR(UINT32, threadId)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    return API::APIReturn();
}

API::APIReturn toggleThreadPin(void *, const API::RESTful::RequestContext &params, Sessions::ClientDetails &clientDetails)
{
    std::unique_lock<std::shared_mutex> lock(g_ctx.dbShrLock);

    uint32_t threadId = Helpers::JSON::ASUINT(*params.inputJSON, "threadId", 0);
    bool pinStatus = Helpers::JSON::ASBOOL(*params.inputJSON, "isPinned", false);
    std::string user = params.jwtToken->getSubject();

    if (threadId == 0)
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_400_BAD_REQUEST, "invalid_request", "Thread ID is required");
    }

    APP_LOG->log2(__func__, user, clientDetails.ipAddress, Logs::INFO, "User is toggling pin for thread %d", threadId);

    if (!g_ctx.dbConnector->qExecuteEx("UPDATE `mboard`.`threads` SET `isPinned`=:isPinned WHERE `threadId`=:threadId;",
                                    {{":isPinned", MAKE_VAR(BOOL, pinStatus)}, {":threadId", MAKE_VAR(UINT32, threadId)}}))
    {
        return API::APIReturn(Network::Protocol::HTTP::Status::Code::S_500_INTERNAL_SERVER_ERROR, "internal_error", "DB Failed");
    }

    return API::APIReturn();
}

// ============================================================================
// API ENDPOINTS REGISTRATION:
// ============================================================================

auto registerAPIEndpoints() -> std::shared_ptr<API::RESTful::Endpoints>
{
    auto endpoints = std::make_shared<API::RESTful::Endpoints>();

    using M = Network::Protocol::HTTP::Method;
    using Sec = API::Security::Requirements;

    // Messageboard endpoints
    endpoints->addEndpoint(M::GET, "threads", Sec::JWT_COOKIE_AUTH, {"READER"}, nullptr, &getThreads);
    endpoints->addEndpoint(M::POST, "threads", Sec::JWT_COOKIE_AUTH, {"WRITER"}, nullptr, &createThread);
    endpoints->addEndpoint(M::GET, "messages", Sec::JWT_COOKIE_AUTH, {"READER"}, nullptr, &getMessages);
    endpoints->addEndpoint(M::POST, "messages", Sec::JWT_COOKIE_AUTH, {"WRITER"}, nullptr, &postMessage);
    endpoints->addEndpoint(M::PUT, "messages", Sec::JWT_COOKIE_AUTH, {"WRITER"}, nullptr, &editMessage);
    endpoints->addEndpoint(M::DELETE, "messages", Sec::JWT_COOKIE_AUTH, {"WRITER"}, nullptr, &deleteMessage);
    endpoints->addEndpoint(M::PUT, "threads/lock", Sec::JWT_COOKIE_AUTH, {"EDITOR"}, nullptr, &toggleThreadLock);
    endpoints->addEndpoint(M::PUT, "threads/pin", Sec::JWT_COOKIE_AUTH, {"EDITOR"}, nullptr, &toggleThreadPin);

    return endpoints;
}
