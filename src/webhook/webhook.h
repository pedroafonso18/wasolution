#pragma once

#include "../constants.h"
#include <curl/curl.h>
#include "webhook_constant.h"

class Webhook {
private:
    static Webhook_t deserialize_w(nlohmann::json original_webhook);
    static Webhook_t deserialize_e(nlohmann::json original_webhook);
    static Status send_webhook(Webhook_t deserialized_webhook, std::string url);
public:
    Webhook() = delete;
    static Status deserialize_webhook( nlohmann::json json_data, ApiType api_type, std::string url);
};