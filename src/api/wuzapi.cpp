#include "wuzapi.h"
#include "logger/logger.h"
#include <thread>

#include "config/config.h"
#include "database/database.h"
using std::string;

extern Logger apiLogger;

Status Wuzapi::setProxy_w(string token, string proxy_url, string url) {
    apiLogger.info("Configurando proxy para instância: " + token);
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/proxy", url);
    string req_hdr = std::format("token: {}", token);
    string req_body = std::format(R"({{"proxy_url": "{}", "enable": true}})", proxy_url);
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
            apiLogger.error("Erro HTTP na configuração do proxy");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Proxy configurado com sucesso");
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

    return stat;
}

Status Wuzapi::getQrCode_w(string token, string url) {
    apiLogger.info("Buscando QR Code para instância: " + token);
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/session/qr", url);
    string req_hdr = std::format("token: {}", token);
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
            apiLogger.error("Erro HTTP ao buscar QR Code");
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
            apiLogger.debug("QRCode vazio encontrado na resposta da API");
            needQrFromDb = true;
        }

        if (needQrFromDb) {
            apiLogger.info("Buscando QR Code no banco de dados");
            Config cfg;
            Database db;
            auto db_url = cfg.getEnv().db_url_wuz;

            if (db.connect(db_url).status_code != c_status::OK) {
                apiLogger.error("Falha ao conectar ao banco de dados");
                return stat;
            }

            apiLogger.debug("Aguardando 500ms antes de buscar o QR Code");
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            auto qrCode = db.getQrCodeFromDB(token);

            if (!qrCode.has_value() || qrCode->empty()) {
                apiLogger.debug("QR Code não encontrado, aguardando mais 1.5 segundos");
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                qrCode = db.getQrCodeFromDB(token);
            }

            if (qrCode.has_value() && !qrCode->empty()) {
                apiLogger.info("QR Code encontrado no banco de dados");

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
                apiLogger.error("QR Code não encontrado no banco de dados para o token: " + token);
            }
        } else {
            apiLogger.info("QR Code já presente na resposta da API");
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
                {"raw_response", responseBody},
                {"error", e.what()}
            };
        }
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
        std::cout << "WEBHOOK-ERROR: CURL NOT INITIALIZED\n";
        return stat;
    }

    const string req_url = std::format("{}/webhook", url);
    string req_hdr = std::format("token: {}", token);
    string req_body = std::format(R"({{"webhook": "{}", "data": ["Message","ReadReceipt","Presence","HistorySync","ChatPresence"]}})", webhook_url);
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

Status Wuzapi::sendMessage_w(string phone, string token, string url, MediaType type, string msg_template) {
    apiLogger.info("Enviando mensagem para número: " + phone);
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
        req_url = std::format("{}/chat/send/text", url);
        req_body = std::format(R"({{"Phone": "{}", "Body": "{}"}})", phone, msg_template);
    } else if (type == MediaType::AUDIO) {
        req_url = std::format("{}/chat/send/audio", url);
        req_body = std::format(R"({{"Phone": "{}", "Audio": "{}"}})", phone, msg_template);
    } else if (type == MediaType::IMAGE) {
        req_url = std::format("{}/chat/send/image", url);
        req_body = std::format(R"({{"Phone": "{}", "Image": "{}", "Caption" : ""}})", phone, msg_template);
    }

    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

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

Status Wuzapi::createInstance_w(string inst_token, string inst_name, string url, string webhook_url, string proxy_url, string wuz_admin_token) {
    Status stat;
    Config cfg;
    Env env = cfg.getEnv();
    CURL *curl = curl_easy_init();
    std::string responseBody;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/admin/users", url);

    nlohmann::json req_body_json;

    req_body_json["name"] = inst_name;
    req_body_json["token"] = inst_token;

    if (!webhook_url.empty()) {
        req_body_json["webhook"] = webhook_url;
        req_body_json["events"] = "All";
    }

    if (!proxy_url.empty()) {
        req_body_json["proxyConfig"] = {
            {"enabled", true},
            {"proxyURL", proxy_url}
        };
    }

    string req_body = req_body_json.dump();

    apiLogger.debug("Creating WuzAPI instance");
    apiLogger.debug("URL: " + req_url);
    apiLogger.debug("Request body: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Authorization: {}", wuz_admin_token);

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
        apiLogger.error("CURL error: " + std::string(curl_easy_strerror(res)));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", curl_easy_strerror(res)}};
        return stat;
    }

    bool http_ok = isHttpResponseOk(curl);
    apiLogger.debug("HTTP Response: " + responseBody);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    try {
        nlohmann::json response = nlohmann::json::parse(responseBody);

        if (!http_ok) {
            apiLogger.error("HTTP error when creating WuzAPI instance");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Server returned HTTP error code";
                stat.status_string = response;
            }
            return stat;
        }

        stat.status_code = c_status::OK;
        stat.status_string = response;
    } catch (const std::exception& e) {
        apiLogger.error("Error parsing response: " + std::string(e.what()));
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

    if (auto conn = connectInstance_w(inst_token, url); conn.status_code == c_status::ERR) {
        return conn;
    }

    return getQrCode_w(inst_token, url);
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

    string req_body = std::format(R"({{"Subscribe": ["Message","ReadReceipt","Presence","HistorySync","ChatPresence"], "Immediate": true}})");

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

Status Wuzapi::deleteInstance_w(string inst_token, string url, string wuz_admin_token) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/admin/users/{}", url, inst_token);

    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Authorization: {}", wuz_admin_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
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
