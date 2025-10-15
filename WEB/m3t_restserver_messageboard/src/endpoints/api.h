#pragma once

#include <Mantids30/Server_RESTfulWebAPI/engine.h>

/**
 * @brief Register all API endpoints
 */
auto registerAPIEndpoints() -> std::shared_ptr<Mantids30::API::RESTful::Endpoints>;


/*
API Documentation for Message Board System

Base URL
https://mywebsite/api/v1/

Authentication
All endpoints require JWT authentication via cookie. Required scopes:
- READER: Read access to threads and messages
- WRITER: Write access to create/edit/delete messages
- EDITOR: Administrative access to lock/pin threads

Endpoints

1. Get Threads List
GET /api/v1/threads

Description: Retrieve all active threads ordered by pinned status and last post time.

Response:
[
  {
    "threadId": 1,
    "title": "Welcome to the forum",
    "creatorUserId": "user123",
    "createdAt": "2023-01-15T10:30:00Z",
    "lastPostAt": "2023-01-20T14:45:00Z",
    "isPinned": true,
    "isLocked": false
  }
]

2. Create New Thread
POST /api/v1/threads

Request Body (JSON):
{
  "title": "New Thread Title"
}

Response: Code 200

3. Get Messages for Thread
GET /api/v1/messages

Query Parameters:
- threadId (required): Thread identifier

Response:
[
  {
    "messageId": 1,
    "userId": "user456",
    "content": "This is the first message",
    "ipAddress": "192.168.1.100",
    "userAgent": "Mozilla/5.0...",
    "createdAt": "2023-01-15T10:35:00Z",
    "editedAt": null
  }
]

4. Post New Message
POST /api/v1/messages

Request Body (JSON):
{
  "threadId": 1,
  "content": "This is my new message"
}

Response: Code 200

5. Edit Message
PUT /api/v1/messages

Request Body (JSON):
{
  "messageId": 1,
  "content": "Updated message content"
}

Response: Code 200

6. Delete Message
DELETE /api/v1/messages

Request Body (JSON):
{
  "messageId": 1
}

Response: Code 200

7. Lock/Unlock Thread
PUT /api/v1/threads/lock

Request Body (JSON):
{
  "threadId": 1,
  "isLocked": true
}

Response: Code 200

8. Pin/Unpin Thread
PUT /api/v1/threads/pin

Request Body (JSON):
{
  "threadId": 1,
  "isPinned": true
}

Response: Code 200

Error Responses

All endpoints return standard HTTP status codes:
- 200: Success
- 400: Bad Request (invalid parameters)
- 401: Unauthorized (missing or invalid authentication)
- 403: Forbidden (insufficient permissions)
- 404: Not Found (resource not found)
- 500: Internal Server Error

Security Notes
- All write operations require administrative privileges for thread management
- Message deletion requires either ownership or admin rights
- Thread locking/pinning can only be performed by editors
- Access tokens must be included in cookie header for all authenticated requests

Rate Limiting
Requests are rate-limited based on IP address and user account to prevent abuse. Excessive requests will result in 429 Too Many Requests responses.

*/
