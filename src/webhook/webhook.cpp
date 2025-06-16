#include "webhook.h"

Webhook_t Webhook::deserialize_e(nlohmann::json original_webhook) {
    Webhook_t webhook;
    if (original_webhook.contains("event") && original_webhook["event"] == "send.message") {

    }
}

Webhook_t Webhook::deserialize_w(nlohmann::json original_webhook) {

}

Status Webhook::send_webhook(Webhook_t deserialized_webhook) {

}

Status Webhook::deserialize_webhook(nlohmann::json original_webhook, ApiType inst_type) {
    Webhook_t webhook;
    Status stat;
    if (inst_type == ApiType::WUZAPI) {
        webhook = deserialize_w(original_webhook);
        Status response = send_webhook(webhook);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                                {"webhook", "Webhook sent successfully!"},
                                {"api_response", response.status_string}
                };
            }
        }
        return response;
    } else if (inst_type == ApiType::EVOLUTION) {
        webhook = deserialize_e(original_webhook);
        Status response = send_webhook(webhook);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                                    {"webhook", "Webhook sent successfully!"},
                                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;
}