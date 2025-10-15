#pragma once

#include "json/value.h"
#include <Mantids30/Helpers/json.h>

static auto parse = [](const char *json)
{
    Json::Value r;
    Json::Reader().parse(json, r);
    return r;
};

Json::Value appScopes()
{
    return parse(R"(
    [
      {
        "id": "READER",
        "description": "Read access to threads and messages"
      },
      {
        "id": "WRITER",
        "description": "Write access to create/edit/delete messages"
      },
      {
        "id": "EDITOR",
        "description": "Administrative access to lock/pin threads"
      }
    ]
    )");
}

Json::Value appRoles()
{
    return parse(R"(
    [
      {
        "id": "BOARDUSER",
        "description": "Have access to READER+WRITER",
        "scopes": [
          "READER",
          "WRITER"
        ]
      },
      {
        "id": "BOARDADMIN",
        "description": "Have access to READER+WRITER+EDITOR",
        "scopes": [
          "READER",
          "WRITER",
          "EDITOR"
        ]
      }
    ]
    )");
}

Json::Value appActivities()
{
    return parse(R"(
    [
      {
        "id": "LOGIN",
        "description": "Application Login Activity",
        "parentActivity": null
      }
    ]
    )");
}
