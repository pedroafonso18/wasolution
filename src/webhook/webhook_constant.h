#pragma once

#include"../../dependencies/json.h"

enum class WebhookType {
    MESSAGE,
    PRESENCE
    // TODO: Add more webhook types, for now these will do.
};

typedef struct {
    WebhookType web_type;
    nlohmann::json web_data;
} Webhook_t;

typedef struct {
    std::string instance_id;
    std::string instance_name;
    std::string webhook_type;
    std::string sender;
    std::string receiver;
    bool from_me;
    std::string date_time;
    std::string message;
} Webhook_message;