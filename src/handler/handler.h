#pragma once

#include <string>
#include "../constants.h"
#include "../api/evolution.h"
#include "../api/wuzapi.h"
#include "../config/config.h"
#include "../database/database.h"
#include "../cloud/cloud_constants.h"

using std::string;


class Handler {
    public:
        Handler() = delete;

        static Status sendMessage(const string &instance_id, string number, string body, MediaType type);
        static Status createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url, std::string access_token, std::string waba_id);
        static Status deleteInstance(string instance_id);
        static Status connectInstance(string instance_id);
        static Status logoutInstance(string instance_id);
        static Status setWebhook(string token, string webhook_url);
        static std::vector<Database::Instance> retrieveInstances();
        static Status sendTemplate(string instance_id, string number, string body, MediaType type, std::vector<FB_VARS> vars, std::string template_name);
        static Status createGroup(string instance_id, string subject, string description, std::vector<string> participants);
};