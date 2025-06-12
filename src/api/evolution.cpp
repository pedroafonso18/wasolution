#include "evolution.h"
using std::string;

Status Evolution::sendMessage_e(string phone, string token, string url, MediaType type, string msg_template, string instance_name) {
    CURL* curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }
    string req_url;
    string req_body;
    std::cout << "Starting operation to send message for Evolution - Sending message to number - " << phone << '\n';
    if (type == MediaType::TEXT) {
        req_url = std::format("{}/message/sendText/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "text" : "{}"}})", phone, msg_template);
    } else if (type == MediaType::AUDIO) {
        req_url = std::format("{}/message/sendWhatsappAudio/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "audio" : "{}", "delay": 100}})", phone, msg_template);
    } else if (type == MediaType::IMAGE) {
        req_url = std::format("{}/message/sendSticker/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "sticker" : "{}"}})", phone, msg_template);
    }
    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", token);
    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_slist_free_all(headers);
        stat.status_code = c_status::OK;
        stat.status_string = curl_easy_strerror(res);
        return stat;
    }

    curl_slist_free_all(headers);
    stat.status_code = c_status::OK;
    stat.status_string = responseBody;
    return stat;
}

Status Evolution::createInstance_e(string evo_token, string inst_token, string inst_name, string url, string webhook_url, Proxy proxy_url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }
    string req_body;
    const string req_url = std::format("{}/instance/create");
    if (!proxy_url.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhookUrl" : "{}", "events" : ["MESSAGES_UPSERT"], "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token, webhook_url, proxy_url.host, proxy_url.port, proxy_url.protocol, proxy_url.username, proxy_url.password);
    } else if (!proxy_url.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token,proxy_url.host, proxy_url.port, proxy_url.protocol, proxy_url.username, proxy_url.password);
    } else if (proxy_url.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhookUrl" : "{}", "events" : ["MESSAGES_UPSERT"]}})", inst_name, inst_token, webhook_url);
    } else if (proxy_url.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}"}})", inst_name, inst_token);
    }
    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_slist_free_all(headers);
        stat.status_code = c_status::OK;
        stat.status_string = curl_easy_strerror(res);
        return stat;
    }

    curl_slist_free_all(headers);
    stat.status_code = c_status::OK;
    stat.status_string = responseBody;
    return stat;
}


Status Evolution::deleteInstance_e(string inst_token, string evo_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }

    const string req_url = std::format("{}/instance/delete/{}", url, inst_token);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        std::cerr << "CURL error: " <<  curl_easy_strerror(res) << '\n';
        curl_slist_free_all(headers);
        stat.status_code = c_status::OK;
        stat.status_string = curl_easy_strerror(res);
        return stat;
    }

    curl_slist_free_all(headers);
    stat.status_code = c_status::OK;
    stat.status_string = responseBody;
    return stat;
}

