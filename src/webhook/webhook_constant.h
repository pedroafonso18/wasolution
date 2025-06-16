#pragma once

#include"../../dependencies/json.h"

enum class WebhookType {
    MESSAGE,
    PRESENCE,
    ERROR
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

void to_json(nlohmann::json& j, const Webhook_message& m) {
    j = nlohmann::json{{"instanceId", m.instance_id}, {"instanceName", m.instance_name}, {"webhookType", m.webhook_type}, {"sender", m.sender}, {"receiver", m.receiver}, {"fromMe", m.from_me}, {"dateTime", m.date_time}, {"message", m.message}};
}