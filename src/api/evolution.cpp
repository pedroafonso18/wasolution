#include "evolution.h"
#include "logger/logger.h"
#include "config/config.h"
using std::string;

extern Logger apiLogger;

Evolution::Proxy Evolution::ParseProxy(std::string proxy_url) {
    apiLogger.debug("Analisando URL do proxy: " + proxy_url);
    Proxy proxy = {"", "", "", "", ""};
    try {
        size_t proto_end = proxy_url.find("://");
        if (proto_end == std::string::npos) {
            apiLogger.error("URL do proxy inválida: protocolo não encontrado");
            return proxy;
        }
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
        apiLogger.debug("Proxy analisado com sucesso: " + proxy.protocol + "://" + proxy.host + ":" + proxy.port);
    } catch (...) {
        apiLogger.error("Erro ao analisar URL do proxy");
        return Proxy{"", "", "", "", ""};
    }
    return proxy;
}

Status Evolution::sendMessage_e(string phone, string token, string url, MediaType type, string msg_template, string instance_name) {
    apiLogger.info("Enviando mensagem para número: " + phone + " via Evolution");
    CURL* curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    string req_url;
    string req_body;
    if (type == MediaType::TEXT) {
        req_url = std::format("{}/message/sendText/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "text" : "{}"}})", phone, msg_template);
    } else if (type == MediaType::AUDIO) {
        req_url = std::format("{}/message/sendWhatsappAudio/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "audio" : "{}", "delay": 100}})", phone, msg_template);
    } else if (type == MediaType::IMAGE) {
        req_url = std::format("{}/message/sendMedia/{}", url, instance_name);
        req_body = std::format(R"({{"number" : "{}", "media" : "{}", "mediatype" : "image", "mimetype" : "image/png", "caption": "", "fileName" : "imagem.png"}})", phone, msg_template);
    }
    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", token);
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
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao enviar mensagem");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Mensagem enviada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta do envio: " + std::string(e.what()));
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    return stat;
}

Status Evolution::createInstance_e(string evo_token, string inst_token, string inst_name, string url, string webhook_url, std::string proxy_url) {
    apiLogger.info("Criando instância Evolution: " + inst_name);
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Config cfg;
    auto env = cfg.getEnv();
    Status stat;
    Proxy prox = ParseProxy(proxy_url);
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    string req_body;
    const string req_url = std::format("{}/instance/create", url);
    if (!prox.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhook": {{"url": "{}", "byEvents": false, "base64": true, "events": ["MESSAGES_UPSERT"]}}, "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token, webhook_url, prox.host, prox.port, prox.protocol, prox.username, prox.password);
    } else if (!prox.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token,prox.host, prox.port, prox.protocol, prox.username, prox.password);
    } else if (prox.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhook": {{"url": "{}", "byEvents": false, "base64": true, "events": ["MESSAGES_UPSERT"]}}}})", inst_name, inst_token, webhook_url);
    } else if (prox.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS"}})", inst_name, inst_token);
    }
    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao criar instância");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância criada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da criação: " + std::string(e.what()));
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    return stat;
}

Status Evolution::deleteInstance_e(string inst_token, string evo_token, string url) {
    apiLogger.info("Deletando instância Evolution");
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    const string req_url = std::format("{}/instance/delete/{}", url, inst_token);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao deletar instância");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância deletada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da deleção: " + std::string(e.what()));
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    return stat;
}

Status Evolution::connectInstance_e(const string& inst_token, const string& evo_url, const string& evo_token) {
    apiLogger.info("Conectando instância Evolution");
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    const string req_url = std::format("{}/instance/connect/{}", evo_url, inst_token);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");

    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao conectar instância");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância conectada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da conexão: " + std::string(e.what()));
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    return stat;
}

Status Evolution::logoutInstance_e(const string& inst_token, const string& evo_url, const string& evo_token) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/instance/logout/{}", evo_url, inst_token);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
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

    bool http_ok = isHttpResponseOk(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    return stat;
}

Status Evolution::setWebhook_e(string token, string webhook_url, string url, string evo_token) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        std::cout << "WEBHOOK-ERROR: CURL NOT INITIALIZED\n";
        return stat;
    }

    const string req_url = std::format("{}/webhook/set/{}", url, token);
    string req_body = std::format(R"({{"enabled": true, "url": "{}", "webhookByEvents": true, "webhookBase64": true, "events": ["APPLICATION_STARTUP"]}})", webhook_url);
    std::cout << "BODY and URL constructed successfully!\n";
    std::cout << "BODY: " << req_body << '\n';
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("apikey: {}", evo_token);
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
        std::cout << stat.status_string << '\n';

        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Erro no servidor remoto"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    std::cout << stat.status_string << '\n';

    return stat;
}