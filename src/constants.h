#pragma once

#include <string>

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
    std::string status_string;
} Status;