#pragma once

#include <map>
#include <string>
#include <vector>

std::map<std::string,std::string> databaseDefinitions()
{
    return {
            { "mboard","message_board.db" }
           };
}

std::vector<std::string_view> getSQLCreateStatements()
{
    return {
    // Table for threads (discussion threads)
    R"(CREATE TABLE IF NOT EXISTS `mboard`.`threads` (
            `threadId`          INTEGER         PRIMARY KEY AUTOINCREMENT,
            `title`             VARCHAR(256)    NOT NULL,
            `creatorUserId`     VARCHAR(256)    NOT NULL,
            `createdAt`         DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
            `lastPostAt`        DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
            `isPinned`          BOOLEAN         NOT NULL DEFAULT FALSE,
            `isLocked`          BOOLEAN         NOT NULL DEFAULT FALSE
        );)",

    // Table for messages/posts
    R"(CREATE TABLE IF NOT EXISTS `mboard`.`messages` (
            `messageId`         INTEGER         PRIMARY KEY AUTOINCREMENT,
            `threadId`          INTEGER         NOT NULL,
            `userId`            VARCHAR(256)    NOT NULL,
            `content`           TEXT            NOT NULL,
            `ipAddress`         VARCHAR(45)     NOT NULL,
            `userAgent`         VARCHAR(512)    DEFAULT NULL,
            `createdAt`         DATETIME        NOT NULL DEFAULT CURRENT_TIMESTAMP,
            `editedAt`          DATETIME        DEFAULT NULL,
            `isDeleted`         BOOLEAN         NOT NULL DEFAULT FALSE,
            FOREIGN KEY (`threadId`) REFERENCES `threads`(`threadId`)
        );)",

    // Indexes for performance
    R"(CREATE INDEX IF NOT EXISTS `mboard`.`idx_threads_lastpost` ON `threads`(`lastPostAt` DESC);)",
    R"(CREATE INDEX IF NOT EXISTS `mboard`.`idx_messages_thread` ON `messages`(`threadId`, `createdAt`);)",
    R"(CREATE INDEX IF NOT EXISTS `mboard`.`idx_messages_user` ON `messages`(`userId`);)"
    };
}
