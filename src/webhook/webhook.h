#pragma once

#include "../constants.h"
#include <curl/curl.h>

class Webhook {
public:
    Webhook() = delete;
    static Status send_webhook(nlohmann::json deserialized_webhook, std::string url);
};