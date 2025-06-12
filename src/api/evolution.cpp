#include "evolution.h"
using std::string;

Evolution::Proxy Evolution::ParseProxy(std::string proxy_url) {
    Proxy proxy = {"", "", "", "", ""};
    try {
        size_t proto_end = proxy_url.find("://");
        if (proto_end == std::string::npos) return proxy;
        proxy.protocol = proxy_url.substr(0, proto_end);

        size_t creds_start = proto_end + 3;
        size_t at_pos = proxy_url.find('@', creds_start);
        size_t host_start = creds_start;

        if (at_pos != std::string::npos) {
            std::string creds = proxy_url.substr(creds_start, at_pos - creds_start);
            size_t colon_pos = creds.find(':');
            if (colon_pos != std::string::npos) {
                proxy.username = creds.substr(0, colon_pos);
                proxy.password = creds.substr(colon_pos + 1);
            } else {
                proxy.username = creds;
            }
            host_start = at_pos + 1;
        }

        size_t colon_pos = proxy_url.find(':', host_start);
        size_t port_start = std::string::npos;
        if (colon_pos != std::string::npos) {
            proxy.host = proxy_url.substr(host_start, colon_pos - host_start);
            port_start = colon_pos + 1;
            size_t slash_pos = proxy_url.find('/', port_start);
            if (slash_pos != std::string::npos) {
                proxy.port = proxy_url.substr(port_start, slash_pos - port_start);
            } else {
                proxy.port = proxy_url.substr(port_start);
            }
        } else {
            size_t slash_pos = proxy_url.find('/', host_start);
            if (slash_pos != std::string::npos) {
                proxy.host = proxy_url.substr(host_start, slash_pos - host_start);
            } else {
                proxy.host = proxy_url.substr(host_start);
            }
        }
    } catch (...) {
        return Proxy{"", "", "", "", ""};
    }
    return proxy;
}

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

Status Evolution::createInstance_e(string evo_token, string inst_token, string inst_name, string url, string webhook_url, std::string proxy_url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    Proxy prox = ParseProxy(proxy_url);
    if (!curl) {
        throw std::cerr << "Failed to initialize CURL\n";
    }
    string req_body;
    const string req_url = std::format("{}/instance/create");
    if (!prox.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhookUrl" : "{}", "events" : ["MESSAGES_UPSERT"], "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token, webhook_url, proxy_url.host, proxy_url.port, proxy_url.protocol, proxy_url.username, proxy_url.password);
    } else if (!prox.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token,proxy_url.host, proxy_url.port, proxy_url.protocol, proxy_url.username, proxy_url.password);
    } else if (prox.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhookUrl" : "{}", "events" : ["MESSAGES_UPSERT"]}})", inst_name, inst_token, webhook_url);
    } else if (prox.host.empty() && webhook_url.empty()) {
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

