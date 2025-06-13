#pragma once

#include "../constants.h"
#include"../../dependencies/json.h"

class Webhook {
public:
    Webhook() = delete;
    static nlohmann::json deserialize_w(nlohmann::json original_webhook);
    static nlohmann::json deserialize_e(nlohmann::json original_webhook);
};