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
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== SEND MESSAGE (EVOLUTION) START ===");
    apiLogger.info("Enviando mensagem para número: " + phone + " via Evolution");
    apiLogger.debug("Tipo de mídia: " + std::to_string(static_cast<int>(type)));
    apiLogger.debug("Instância: " + instance_name);
    
    if (phone.empty()) {
        apiLogger.error("Número de telefone inválido: phone está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid phone number: phone is empty"}}};
    }
    if (token.empty()) {
        apiLogger.error("Token inválido: token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid token: token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    if (msg_template.empty()) {
        apiLogger.error("Template da mensagem inválido: msg_template está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid message template: msg_template is empty"}}};
    }
    if (instance_name.empty()) {
        apiLogger.error("Nome da instância inválido: instance_name está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance name: instance_name is empty"}}};
    }
    
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
    nlohmann::json req_body_json;
    if (type == MediaType::TEXT) {
        req_url = std::format("{}/message/sendText/{}", url, instance_name);
        req_body_json = {
            {"number", phone},
            {"text", msg_template}
        };
        apiLogger.debug("Enviando mensagem de texto");
    } else if (type == MediaType::AUDIO) {
        req_url = std::format("{}/message/sendWhatsappAudio/{}", url, instance_name);
        req_body_json = {
            {"number", phone},
            {"audio", msg_template},
            {"delay", 100}
        };
        apiLogger.debug("Enviando mensagem de áudio");
    } else if (type == MediaType::IMAGE) {
        req_url = std::format("{}/message/sendMedia/{}", url, instance_name);
        
        std::string media_data = msg_template;
        if (!media_data.empty() && media_data.substr(0, 22) == "data:image/png;base64,") {
            media_data = media_data.substr(22);
            apiLogger.debug("Removed data URL prefix from base64 data");
        } else if (!media_data.empty() && media_data.substr(0, 5) == "data:") {
            size_t comma_pos = media_data.find(',');
            if (comma_pos != std::string::npos) {
                media_data = media_data.substr(comma_pos + 1);
                apiLogger.debug("Removed data URL prefix from base64 data");
            }
        }
        
        req_body_json = {
            {"number", phone},
            {"media", media_data},
            {"mediatype", "image"},
            {"mimetype", "image/png"},
            {"caption", ""},
            {"fileName", "imagem.png"}
        };
        apiLogger.debug("Enviando mensagem de imagem");
        apiLogger.debug("Media data length: " + std::to_string(media_data.length()));
        if (media_data.length() > 50) {
            apiLogger.debug("Media data preview: " + media_data.substr(0, 50) + "...");
        } else {
            apiLogger.debug("Media data: " + media_data);
        }
    } else {
        apiLogger.error("Tipo de mídia não suportado: " + std::to_string(static_cast<int>(type)));
        return Status{c_status::ERR, nlohmann::json{{"error", "Unsupported media type"}}};
    }
    
    string req_body = req_body_json.dump();
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

    apiLogger.debug("Executando requisição CURL para envio de mensagem...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao enviar mensagem - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Mensagem enviada com sucesso para número: " + phone);
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== SEND MESSAGE (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Evolution::createInstance_e(string evo_token, string inst_token, string inst_name, string url, string webhook_url, std::string proxy_url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== CREATE INSTANCE (EVOLUTION) START ===");
    apiLogger.info("Criando instância Evolution: " + inst_name);
    apiLogger.debug("Token Evolution: " + evo_token);
    apiLogger.debug("Token da instância: " + inst_token);
    apiLogger.debug("URL da API: " + url);
    apiLogger.debug("URL do webhook: " + webhook_url);
    apiLogger.debug("URL do proxy: " + proxy_url);
    
    if (evo_token.empty()) {
        apiLogger.error("Token Evolution inválido: evo_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution token: evo_token is empty"}}};
    }
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (inst_name.empty()) {
        apiLogger.error("Nome da instância inválido: inst_name está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance name: inst_name is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
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
        apiLogger.debug("Webhook e proxy configurados para a instância");
    } else if (!prox.host.empty() && webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "proxyHost": "{}", "proxyPort": "{}", "proxyProtocol" : "{}",  "proxyUsername" : "{}", "proxyPassword" : "{}"}})", inst_name, inst_token,prox.host, prox.port, prox.protocol, prox.username, prox.password);
        apiLogger.debug("Proxy configurado para a instância");
    } else if (prox.host.empty() && !webhook_url.empty()) {
        req_body = std::format(R"({{"instanceName" : "{}","token" : "{}", "integration": "WHATSAPP-BAILEYS", "qrcode" : true, "webhook": {{"url": "{}", "byEvents": false, "base64": true, "events": ["MESSAGES_UPSERT"]}}}})", inst_name, inst_token, webhook_url);
        apiLogger.debug("Webhook configurado para a instância");
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

    apiLogger.debug("Executando requisição CURL para criação da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao criar instância - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância criada com sucesso: " + inst_name);
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== CREATE INSTANCE (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Evolution::deleteInstance_e(string inst_token, string evo_token, string url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== DELETE INSTANCE (EVOLUTION) START ===");
    apiLogger.info("Deletando instância Evolution: " + inst_token);
    apiLogger.debug("Token Evolution: " + evo_token);
    apiLogger.debug("URL da API: " + url);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (evo_token.empty()) {
        apiLogger.error("Token Evolution inválido: evo_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution token: evo_token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
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

    apiLogger.debug("Executando requisição CURL para deleção da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao deletar instância - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância deletada com sucesso: " + inst_token);
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== DELETE INSTANCE (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Evolution::connectInstance_e(const string& inst_token, const string& evo_url, const string& evo_token) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== CONNECT INSTANCE (EVOLUTION) START ===");
    apiLogger.info("Conectando instância Evolution: " + inst_token);
    apiLogger.debug("Token Evolution: " + evo_token);
    apiLogger.debug("URL Evolution: " + evo_url);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (evo_url.empty()) {
        apiLogger.error("URL Evolution inválida: evo_url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution URL: evo_url is empty"}}};
    }
    if (evo_token.empty()) {
        apiLogger.error("Token Evolution inválido: evo_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution token: evo_token is empty"}}};
    }
    
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

    apiLogger.debug("Executando requisição CURL para conexão da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao conectar instância - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância conectada com sucesso: " + inst_token);
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== CONNECT INSTANCE (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Evolution::logoutInstance_e(const string& inst_token, const string& evo_url, const string& evo_token) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== LOGOUT INSTANCE (EVOLUTION) START ===");
    apiLogger.info("Desconectando instância Evolution: " + inst_token);
    apiLogger.debug("Token Evolution: " + evo_token);
    apiLogger.debug("URL Evolution: " + evo_url);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (evo_url.empty()) {
        apiLogger.error("URL Evolution inválida: evo_url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution URL: evo_url is empty"}}};
    }
    if (evo_token.empty()) {
        apiLogger.error("Token Evolution inválido: evo_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution token: evo_token is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/instance/logout/{}", evo_url, inst_token);
    apiLogger.debug("URL da requisição: " + req_url);

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

    apiLogger.debug("Executando requisição CURL para desconexão da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP ao desconectar instância - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância desconectada com sucesso: " + inst_token);
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da desconexão: " + std::string(e.what()));
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== LOGOUT INSTANCE (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Evolution::setWebhook_e(string token, string webhook_url, string url, string evo_token) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== SET WEBHOOK (EVOLUTION) START ===");
    apiLogger.info("Configurando webhook Evolution para token: " + token);
    apiLogger.debug("URL do webhook: " + webhook_url);
    apiLogger.debug("URL Evolution: " + url);
    apiLogger.debug("Token Evolution: " + evo_token);
    
    if (token.empty()) {
        apiLogger.error("Token inválido: token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid token: token is empty"}}};
    }
    if (webhook_url.empty()) {
        apiLogger.error("URL do webhook inválida: webhook_url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid webhook URL: webhook_url is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL Evolution inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution URL: url is empty"}}};
    }
    if (evo_token.empty()) {
        apiLogger.error("Token Evolution inválido: evo_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid Evolution token: evo_token is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        std::cout << "WEBHOOK-ERROR: CURL NOT INITIALIZED\n";
        return stat;
    }

    const string req_url = std::format("{}/webhook/set/{}", url, token);
    string req_body = std::format(R"({{"enabled": true, "url": "{}", "webhookByEvents": true, "webhookBase64": true, "events": ["APPLICATION_STARTUP"]}})", webhook_url);
    apiLogger.debug("BODY: " + req_body);
    apiLogger.debug("URL: " + req_url);

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

    apiLogger.debug("Executando requisição CURL para configuração do webhook...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        std::cout << stat.status_string << '\n';
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    bool http_ok = isHttpResponseOk(curl);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP na configuração do webhook - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Webhook Evolution configurado com sucesso para token: " + token);
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da configuração do webhook: " + std::string(e.what()));
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

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== SET WEBHOOK (EVOLUTION) END - Duração: " + std::to_string(duration.count()) + "ms ===");

    std::cout << stat.status_string << '\n';
    return stat;
}