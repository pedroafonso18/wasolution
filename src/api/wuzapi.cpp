#include "wuzapi.h"

#include "config/config.h"
using std::string;


Status Wuzapi::setProxy_w(string token, string proxy_url, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/proxy", url);
    string req_hdr = std::format("token : {}", token);
    string req_body = std::format(R"({{"proxy_url": "{}", "enable": true}})", proxy_url);
    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, req_hdr.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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

Status Wuzapi::setWebhook_w(string token, string webhook_url, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/webhook", url);
    string req_hdr = std::format("token : {}", token);
    string req_body = std::format(R"({{"webhook": "{}", "data": "["Message", "ReadReceipt"]"}})", webhook_url);
    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, req_hdr.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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


Status Wuzapi::sendMessage_w(string phone, string token, string url, MediaType type, string msg_template) {
    CURL* curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
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

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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

Status Wuzapi::createInstance_w(string inst_token, string url, string webhook_url, string proxy_url) {
    Status stat;

    if (!proxy_url.empty()) {
        stat = setProxy_w(inst_token, proxy_url, url);
        if (stat.status_code == c_status::ERR) {
            return stat;
        }
    }

    if (!webhook_url.empty()) {
        stat = setWebhook_w(inst_token, webhook_url, url);
        if (stat.status_code == c_status::ERR) {
            return stat;
        }
    }

    if (!proxy_url.empty()) {
        stat = setProxy_w(inst_token, proxy_url, url);
        if (stat.status_code == c_status::ERR) {
            return stat;
        }
    }

    return connectInstance_w(inst_token, url);
}

Status Wuzapi::deleteInstance_w(string inst_token, string wuz_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/instance/delete", url);
    string req_body = std::format(R"({{"token" : "{}"}})", inst_token);

    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("token: {}", wuz_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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

Status Wuzapi::connectInstance_w(string inst_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/session/connect", url);
    string header_auth = std::format("token: {}", inst_token);

    string req_body = std::format(R"({{"Subscribe": ["Message", "ChatPresence"], "Immediate": true}})");

    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, header_auth.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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

Status Wuzapi::logoutInstance_w(string inst_token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/session/disconnect", url);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("token: {}", inst_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

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
