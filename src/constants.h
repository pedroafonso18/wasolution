#pragma once

#include "../dependencies/json.h"

enum class MediaType {
    IMAGE,
    AUDIO,
    TEXT,
    DOCUMENT
};

enum class ApiType {
    EVOLUTION,
    WUZAPI,
    CLOUD
};

enum class c_status {
    OK,
    ERR
};

typedef struct {
    c_status status_code;
    nlohmann::json status_string;
} Status;