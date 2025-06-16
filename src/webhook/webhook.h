#pragma once

#include "../constants.h"
#include "webhook_constant.h"

class Webhook {
private:
    static Webhook_t deserialize_w(nlohmann::json original_webhook);
    static Webhook_t deserialize_e(nlohmann::json original_webhook);
    static Status send_webhook(Webhook_t deserialized_webhook);
public:
    Webhook() = delete;
    static Status deserialize_webhook(nlohmann::json original_webhook, ApiType inst_type);
};