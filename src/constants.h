#pragma once

#include "../dependencies/json.h"

#define PORT 8080
#define IP "0.0.0.0"

enum class MediaType {
    IMAGE,
    AUDIO,
    TEXT
};

enum class ApiType {
    EVOLUTION,
    WUZAPI
};

enum class c_status {
    OK,
    ERR
};

typedef struct {
    c_status status_code;
    nlohmann::json status_string;
} Status;