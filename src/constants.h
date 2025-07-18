#pragma once

#include "../dependencies/json.h"

/* --- CHANGEABLE DATA BELOW --- */

#define PORT 8080
#define IP "0.0.0.0"
#define TOKEN "ABCD1234"

/* --- END OF CHANGEABLE DATA --- */

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