#include "wuzapi.h"
#include "logger/logger.h"
#include <thread>
#include <chrono>

#include "config/config.h"
#include "database/database.h"
using std::string;

extern Logger apiLogger;

Status Wuzapi::setProxy_w(string token, string proxy_url, string url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== SET PROXY START ===");
    apiLogger.info("Configurando proxy para instância: " + token);
    apiLogger.debug("URL da API: " + url);
    apiLogger.debug("URL do proxy: " + proxy_url);
    
    if (token.empty()) {
        apiLogger.error("Token inválido: token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid token: token is empty"}}};
    }
    if (proxy_url.empty()) {
        apiLogger.error("URL do proxy inválida: proxy_url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid proxy URL: proxy_url is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para configuração do proxy");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/proxy", url);
    string req_hdr = fmt::format("token: {}", token);
    string req_body = fmt::format(R"({{"proxy_url": "{}", "enable": true}})", proxy_url);
    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

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

    apiLogger.debug("Executando requisição CURL para configuração do proxy...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na configuração do proxy: " + std::string(curl_easy_strerror(res)));
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
            apiLogger.error("Erro HTTP na configuração do proxy - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Proxy configurado com sucesso para instância: " + token);
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta do proxy: " + std::string(e.what()));
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
    apiLogger.info("=== SET PROXY END - Duração: " + std::to_string(duration.count()) + "ms ===");
    
    return stat;
}

Status Wuzapi::getQrCode_w(string token, string url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== GET QR CODE START ===");
    apiLogger.info("Buscando QR Code para instância: " + token);
    
    if (token.empty()) {
        apiLogger.error("Token inválido: token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid token: token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para busca do QR Code");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/session/qr", url);
    string req_hdr = fmt::format("token: {}", token);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");
    headers = curl_slist_append(headers, req_hdr.c_str());

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "GET");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    apiLogger.debug("Executando requisição CURL para busca do QR Code...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na busca do QR Code: " + std::string(curl_easy_strerror(res)));
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
            apiLogger.error("Erro HTTP ao buscar QR Code - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
            return stat;
        }

        stat.status_code = c_status::OK;
        stat.status_string = response;

        bool needQrFromDb = false;

        if (response.contains("data") && response["data"].is_object() &&
            response["data"].contains("QRCode") &&
            (response["data"]["QRCode"].empty() || response["data"]["QRCode"] == "")) {
            apiLogger.debug("QRCode vazio encontrado na resposta da API - buscando no banco de dados");
            needQrFromDb = true;
        }

        if (needQrFromDb) {
            apiLogger.info("Iniciando busca do QR Code no banco de dados para token: " + token);
            Config cfg;
            Database db;
            auto db_url = cfg.getEnv().db_url_wuz;

            if (db.connect(db_url).status_code != c_status::OK) {
                apiLogger.error("Falha ao conectar ao banco de dados para busca do QR Code");
                return stat;
            }

            apiLogger.debug("Aguardando 500ms antes de buscar o QR Code no banco...");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            auto qrCode = db.getQrCodeFromDB(token);

            if (!qrCode.has_value() || qrCode->empty()) {
                apiLogger.debug("QR Code não encontrado na primeira tentativa, aguardando mais 1.5 segundos");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                qrCode = db.getQrCodeFromDB(token);
            }

            if (qrCode.has_value() && !qrCode->empty()) {
                apiLogger.info("QR Code encontrado no banco de dados para token: " + token);

                if (response.contains("data") && response["data"].is_object()) {
                    if (response["data"].contains("QRCode")) {
                        response["data"]["QRCode"] = qrCode.value();
                    } else {
                        response["data"]["QRCode"] = qrCode.value();
                    }
                } else {
                    response["data"] = {{"QRCode", qrCode.value()}};
                }

                stat.status_string = response;
            } else {
                apiLogger.error("QR Code não encontrado no banco de dados após múltiplas tentativas para o token: " + token);
            }
        } else {
            apiLogger.info("QR Code já presente na resposta da API para token: " + token);
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta do QR Code: " + std::string(e.what()));
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
    apiLogger.info("=== GET QR CODE END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Wuzapi::setWebhook_w(string token, string webhook_url, string url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== SET WEBHOOK START ===");
    apiLogger.info("Configurando webhook para instância: " + token);
    apiLogger.debug("URL do webhook: " + webhook_url);
    
    if (token.empty()) {
        apiLogger.error("Token inválido: token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid token: token is empty"}}};
    }
    if (webhook_url.empty()) {
        apiLogger.error("URL do webhook inválida: webhook_url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid webhook URL: webhook_url is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para configuração do webhook");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/webhook", url);
    string req_hdr = fmt::format("token: {}", token);
    string req_body = fmt::format(R"({{"webhook": "{}", "data": ["Message","ReadReceipt","Presence","HistorySync","ChatPresence"]}})", webhook_url);
    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

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

    apiLogger.debug("Executando requisição CURL para configuração do webhook...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na configuração do webhook: " + std::string(curl_easy_strerror(res)));
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
            apiLogger.error("Erro HTTP na configuração do webhook - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Webhook configurado com sucesso para instância: " + token);
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
    apiLogger.info("=== SET WEBHOOK END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Wuzapi::sendMessage_w(string phone, string token, string url, MediaType type, string msg_template) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== SEND MESSAGE START ===");
    apiLogger.info("Enviando mensagem para número: " + phone);
    apiLogger.debug("Tipo de mídia: " + std::to_string(static_cast<int>(type)));
    
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
    
    CURL* curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para envio de mensagem");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    string req_url;
    string req_body;
    if (type == MediaType::TEXT) {
        req_url = fmt::format("{}/chat/send/text", url);
        req_body = fmt::format(R"({{"Phone": "{}", "Body": "{}"}})", phone, msg_template);
        apiLogger.debug("Enviando mensagem de texto");
    } else if (type == MediaType::AUDIO) {
        req_url = fmt::format("{}/chat/send/audio", url);
        req_body = fmt::format(R"({{"Phone": "{}", "Audio": "{}"}})", phone, msg_template);
        apiLogger.debug("Enviando mensagem de áudio");
    } else if (type == MediaType::IMAGE) {
        req_url = fmt::format("{}/chat/send/image", url);
        req_body = fmt::format(R"({{"Phone": "{}", "Image": "{}", "Caption" : ""}})", phone, msg_template);
        apiLogger.debug("Enviando mensagem de imagem");
    } else {
        apiLogger.error("Tipo de mídia não suportado: " + std::to_string(static_cast<int>(type)));
        return Status{c_status::ERR, nlohmann::json{{"error", "Unsupported media type"}}};
    }

    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

    struct curl_slist *headers = nullptr;
    string authorization = fmt::format("token: {}", token);
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
        apiLogger.error("Erro CURL no envio de mensagem: " + std::string(curl_easy_strerror(res)));
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
    apiLogger.info("=== SEND MESSAGE END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Wuzapi::createInstance_w(string inst_token, string inst_name, string url, string webhook_url, string proxy_url, string wuz_admin_token) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== CREATE INSTANCE START ===");
    apiLogger.info("Criando instância WuzAPI: " + inst_name);
    apiLogger.debug("Token da instância: " + inst_token);
    apiLogger.debug("URL da API: " + url);
    apiLogger.debug("URL do webhook: " + webhook_url);
    apiLogger.debug("URL do proxy: " + proxy_url);
    
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
    if (wuz_admin_token.empty()) {
        apiLogger.error("Token de administrador inválido: wuz_admin_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid admin token: wuz_admin_token is empty"}}};
    }
    
    Status stat;
    Config cfg;
    Env env = cfg.getEnv();
    CURL *curl = curl_easy_init();
    std::string responseBody;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para criação da instância");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/admin/users", url);

    nlohmann::json req_body_json;

    req_body_json["name"] = inst_name;
    req_body_json["token"] = inst_token;

    if (!webhook_url.empty()) {
        req_body_json["webhook"] = webhook_url;
        req_body_json["events"] = "All";
        apiLogger.debug("Webhook configurado para a instância");
    }

    if (!proxy_url.empty()) {
        req_body_json["proxyConfig"] = {
            {"enabled", true},
            {"proxyURL", proxy_url}
        };
        apiLogger.debug("Proxy configurado para a instância");
    }

    string req_body = req_body_json.dump();

    apiLogger.debug("Criando instância WuzAPI");
    apiLogger.debug("URL: " + req_url);
    apiLogger.debug("Request body: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = fmt::format("Authorization: {}", wuz_admin_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req_body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    apiLogger.debug("Executando requisição CURL para criação da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na criação da instância: " + std::string(curl_easy_strerror(res)));
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
    apiLogger.debug("HTTP Response: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("Erro HTTP na criação da instância - Código: " + std::to_string(http_code));
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Server returned HTTP error code";
                stat.status_string = response;
            }
            return stat;
        }

        apiLogger.info("Instância WuzAPI criada com sucesso: " + inst_name);
        stat.status_code = c_status::OK;
        stat.status_string = response;
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta da criação da instância: " + std::string(e.what()));
        if (!http_ok) {
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{
                {"error", "Error on remote server"},
                {"raw_response", responseBody}
            };
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
            };
        }
    }

    apiLogger.info("Conectando instância após criação...");
    if (auto conn = connectInstance_w(inst_token, url); conn.status_code == c_status::ERR) {
        apiLogger.error("Falha ao conectar instância após criação");
        return conn;
    }

    apiLogger.info("Buscando QR Code para instância recém-criada...");
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== CREATE INSTANCE END - Duração: " + std::to_string(duration.count()) + "ms ===");
    
    return getQrCode_w(inst_token, url);
}

Status Wuzapi::connectInstance_w(string inst_token, string url) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== CONNECT INSTANCE START ===");
    apiLogger.info("Conectando instância WuzAPI: " + inst_token);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para conexão da instância");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/session/connect", url);
    string header_auth = fmt::format("token: {}", inst_token);

    string req_body = fmt::format(R"({{"Subscribe": ["Message","ReadReceipt","Presence","HistorySync","ChatPresence"], "Immediate": true}})");

    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

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

    apiLogger.debug("Executando requisição CURL para conexão da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na conexão da instância: " + std::string(curl_easy_strerror(res)));
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
            apiLogger.error("Erro HTTP na conexão da instância - Código: " + std::to_string(http_code));
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
        apiLogger.error("Erro ao processar resposta da conexão da instância: " + std::string(e.what()));
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
    apiLogger.info("=== CONNECT INSTANCE END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Wuzapi::logoutInstance_w(string inst_token, string url) {
    const auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== LOGOUT INSTANCE START ===");
    apiLogger.info("Desconectando instância WuzAPI: " + inst_token);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para desconexão da instância");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/session/disconnect", url);

    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = fmt::format("token: {}", inst_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    apiLogger.debug("Executando requisição CURL para desconexão da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na desconexão da instância: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    stat.status_code = c_status::OK;

    try {
        stat.status_string = nlohmann::json::parse(responseBody);
        apiLogger.info("Instância desconectada com sucesso: " + inst_token);
    } catch (const std::exception& e) {
        apiLogger.warn("Erro ao processar resposta da desconexão, usando resposta bruta: " + std::string(e.what()));
        stat.status_string = nlohmann::json{
            {"raw_response", responseBody}
        };
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== LOGOUT INSTANCE END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}

Status Wuzapi::deleteInstance_w(string inst_token, string url, string wuz_admin_token) {
    auto start_time = std::chrono::high_resolution_clock::now();
    apiLogger.info("=== DELETE INSTANCE START ===");
    apiLogger.info("Deletando instância WuzAPI: " + inst_token);
    
    if (inst_token.empty()) {
        apiLogger.error("Token da instância inválido: inst_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid instance token: inst_token is empty"}}};
    }
    if (url.empty()) {
        apiLogger.error("URL da API inválida: url está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid API URL: url is empty"}}};
    }
    if (wuz_admin_token.empty()) {
        apiLogger.error("Token de administrador inválido: wuz_admin_token está vazio");
        return Status{c_status::ERR, nlohmann::json{{"error", "Invalid admin token: wuz_admin_token is empty"}}};
    }
    
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL para deleção da instância");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = fmt::format("{}/admin/users/{}", url, inst_token);

    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = fmt::format("Authorization: {}", wuz_admin_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);

    apiLogger.debug("Executando requisição CURL para deleção da instância...");
    if (const CURLcode res = curl_easy_perform(curl); res != CURLE_OK) {
        apiLogger.error("Erro CURL na deleção da instância: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    apiLogger.info("Código de resposta HTTP: " + std::to_string(http_code));
    apiLogger.debug("Resposta HTTP: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    stat.status_code = c_status::OK;

    try {
        stat.status_string = nlohmann::json::parse(responseBody);
        apiLogger.info("Instância deletada com sucesso: " + inst_token);
    } catch (const std::exception& e) {
        apiLogger.warn("Erro ao processar resposta da deleção, usando resposta bruta: " + std::string(e.what()));
        stat.status_string = nlohmann::json{
                {"raw_response", responseBody}
        };
    }

    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);
    apiLogger.info("=== DELETE INSTANCE END - Duração: " + std::to_string(duration.count()) + "ms ===");

    return stat;
}
