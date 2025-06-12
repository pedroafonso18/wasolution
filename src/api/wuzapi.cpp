#include "wuzapi.h"
using std::string;

Status Wuzapi::sendMessage_w(string phone, string token, string url, MediaType type, string msg_template) {
    CURL* curl = curl_easy_init();
    std::string responseBody;
    Status stat;

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


Status Wuzapi::createInstance_w(string wuz_token, string inst_token, string inst_name, string url, string webhook_url, string proxy_url ) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;


    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }
    string req_body;
    const string req_url = std::format("{}/admin/users");
    if (!proxy_url.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"name":"{}","token": "{}", "webhook" : "{}", "events": "All", "proxyConfig" : "{"enabled": true, "proxyURL" : "{}"}"}})",inst_name, inst_token, webhook_url, proxy_url);
    } else if (!proxy_url.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"name": "{}", "token": "{}", "proxyConfig" : "{"enabled": true, "proxyURL" : "{}"}"})", inst_name, inst_token, proxy_url);
    } else if (proxy_url.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"name" : "{}", "token": "{}", "webhook" : "{}", "events" : "All"}})", inst_name, inst_token, webhook_url);
    } else {
        req_body = std::format(R"({{"name" : "{}", "token": "{}"}})", inst_name, inst_token);
    }

    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("token: {}", wuz_token);

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

Status Wuzapi::deleteInstance_w(string inst_token, string wuz_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;


    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }

    const string req_url = std::format("{}/admin/users/{}", url, inst_token);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    string authorization = std::format("token: {}", wuz_token);

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

Status Wuzapi::connectInstance_w(string inst_token, string wuz_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;


    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }

    const string req_url = std::format("{}/session/connect/{}", url, inst_token);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    string authorization = std::format("token: {}", wuz_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
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
