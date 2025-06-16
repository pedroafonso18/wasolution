#include "webhook.h"
#include "../api/api_constants.h"
#include <iostream>

Webhook_t Webhook::deserialize_e(nlohmann::json original_webhook) {
    Webhook_t webhook;
    if (original_webhook.contains("event") && original_webhook["event"] == "send.message") {
        try {
            Webhook_message msg;
            msg.webhook_type = "message";
            msg.from_me = original_webhook["data"]["key"]["fromMe"];
            msg.date_time = original_webhook["date_time"];
            msg.instance_id = original_webhook["data"]["instanceId"];
            msg.instance_name = original_webhook["instance"];
            msg.message = original_webhook["data"]["message"]["conversation"];
            msg.sender = original_webhook["data"]["sender"];
            msg.receiver = original_webhook["data"]["key"]["remoteJid"];
            const nlohmann::json j = msg;
            webhook.web_data = j;
            webhook.web_type = WebhookType::MESSAGE;
            return webhook;
        } catch (const std::exception& e) {
            webhook.web_data = {{"error", "Exception when deserializing the webhook."}};
            webhook.web_type = WebhookType::W_ERROR;
            return webhook;
        }
    } else {
        webhook.web_data = {{"error", "Webhook type not supported, currently supported types: MESSAGE."}};
        webhook.web_type = WebhookType::W_ERROR;
        return webhook;
        }
    }

Webhook_t Webhook::deserialize_w(nlohmann::json original_webhook)  {
    Webhook_t webhook;
    if (original_webhook.contains("type") && original_webhook["type"] == "Message") {
        try {
            Webhook_message msg;
            msg.webhook_type = "message";
            msg.from_me = original_webhook["event"]["Info"]["isFromMe"];
            msg.date_time = original_webhook["event"]["Info"]["Timestamp"];
            msg.instance_id = original_webhook["token"];
            msg.instance_name = original_webhook["instance_name"];
            msg.message = original_webhook["event"]["Message"]["extendedTextMessage"]["text"];
            msg.sender = original_webhook["event"]["Info"]["Sender"];
            msg.receiver = original_webhook["event"]["Info"]["pushName"];
            const nlohmann::json j = msg;
            webhook.web_data = j;
            webhook.web_type = WebhookType::MESSAGE;
            return webhook;
        } catch (const std::exception& e) {
            webhook.web_data = {{"error", "Exception when deserializing the webhook."}};
            webhook.web_type = WebhookType::W_ERROR;
            return webhook;
        }
    } else {
        webhook.web_data = {{"error", "Webhook type not supported, currently supported types: MESSAGE."}};
        webhook.web_type = WebhookType::W_ERROR;
        return webhook;
    }
}

Status Webhook::send_webhook(Webhook_t deserialized_webhook, std::string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{ { "error", "Failed to initialize CURL"}};
        return stat;
    }

    std::string req_body = deserialized_webhook.web_data.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    curl_easy_cleanup(curl);
    stat.status_code = c_status::OK;

    try {
        stat.status_string = nlohmann::json::parse(responseBody);
    } catch (const std::exception& e) {
        stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
        };
    }

    return stat;
}

Status Webhook::deserialize_webhook(nlohmann::json json_data, ApiType api_type, std::string url) {
    Webhook_t webhook;
    Status stat;
    if (api_type == ApiType::WUZAPI) {
        webhook = deserialize_w(json_data);
        Status response = send_webhook(webhook, url);
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
    } else if (api_type == ApiType::EVOLUTION) {
        webhook = deserialize_e(json_data);
        Status response = send_webhook(webhook, url);
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