#pragma once

#include <string>
#include "../constants.h"
#include "../webhook/webhook.h"
#include "../api/evolution.h"
#include "../api/wuzapi.h"
#include "../config/config.h"
#include "../database/database.h"

using std::string;


class Handler {
    public:
        Handler() = delete;

        static Status sendMessage(const string &instance_id, string number, string body, MediaType type);
        static Status createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url);
        static Status deleteInstance(string instance_id);
        static Status connectInstance(string instance_id);
        static Status sendWebhook(nlohmann::json webhook, string inst_id);
};