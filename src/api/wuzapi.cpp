#include "wuzapi.h"
using std::string;

Status Wuzapi::sendMessage_w(string phone, string token, string url, MediaType type, string msg_template) {
    CURL* curl = curl_easy_init();
    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }
    string req_url;
    string req_body;
    std::cout << "Starting operation to send message for Wuzapi - Sending message to number - " << phone << '\n';
    if (type == MediaType::TEXT) {
        req_url = std::format("{}/chat/send/text", url);
        req_body = std::format(R"({{"Phone": "{}", "Body": "{}"}})", phone, msg_template);
    } else if (type == MediaType::AUDIO) {
        req_url = std::format("{}/chat/send/audio", url);
        req_body = std::format(R"({{"Phone": "{}", "Audio": "{}"}})", phone, msg_template);
    } else if (type == MediaType::IMAGE) {
        req_url = std::format("{}/chat/send/sticker", url);
        req_body = std::format(R"({{"Phone": "{}", "Sticker": "{}"}})", phone, msg_template);
    }

    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    string authorization = std::format("token: {}", token);
    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_slist_free_all(headers);
        return Status::ERR;
    }

    curl_slist_free_all(headers);
    return Status::OK;
}