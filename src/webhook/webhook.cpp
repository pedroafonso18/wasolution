#include "webhook.h"
#include "../api/api_constants.h"
#include <iostream>


Status Webhook::send_webhook(nlohmann::json deserialized_webhook, std::string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{ { "error", "Failed to initialize CURL"}};
        return stat;
    }

    std::string req_body = deserialized_webhook.dump();

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);

    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    curl_slist_free_all(headers);
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
