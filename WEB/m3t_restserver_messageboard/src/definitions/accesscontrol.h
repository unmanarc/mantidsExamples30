#pragma once

#include <Mantids30/DataFormat_JSON/json.h>
#include <boost/json.hpp>

static auto parse = [](const char *json)
{
    return boost::json::parse(json);
};

boost::json::value appScopes()
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

boost::json::value appRoles()
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

boost::json::value appActivities()
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
