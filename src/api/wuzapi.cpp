#include "wuzapi.h"

#include <thread>

#include "config/config.h"
#include "database/database.h"
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
    string req_hdr = std::format("token: {}", token);
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

Status Wuzapi::getQrCode_w(string token, string url) {
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;

    if (!curl) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    const string req_url = std::format("{}/session/qr", url);
    string req_hdr = std::format("token: {}", token);
    std::cout << "URL constructed successfully!\n";
    std::cout << "URL: " << req_url << '\n';

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
        std::cout << "Resposta recebida da API: " << responseBody << std::endl;

        if (!http_ok) {
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

        std::cout << "Estrutura da resposta: ";
        for (auto& [key, value] : response.items()) {
            std::cout << "Chave: " << key << ", ";
        }
        std::cout << std::endl;

        bool needQrFromDb = false;

        if (response.contains("data") && response["data"].is_object() &&
            response["data"].contains("QRCode") &&
            (response["data"]["QRCode"].empty() || response["data"]["QRCode"] == "")) {
            std::cout << "Caso 1: QRCode vazio encontrado em data.QRCode" << std::endl;
            needQrFromDb = true;
        }

        if (needQrFromDb) {
            std::cout << "QR Code não encontrado ou vazio na resposta da API, buscando no banco de dados...\n";
            Config cfg;
            Database db;
            auto db_url = cfg.getEnv().db_url_wuz;
            std::cout << "Conectando ao banco de dados com URL: " << db_url << std::endl;

            if (db.connect(db_url).status_code != c_status::OK) {
                std::cerr << "Falha ao conectar ao banco de dados\n";
                return stat;
            }

            std::cout << "Aguardando 500ms antes de buscar o QR Code..." << std::endl;
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

            std::cout << "Buscando QR Code para o token: " << token << std::endl;
            auto qrCode = db.getQrCodeFromDB(token);

            if (!qrCode.has_value() || qrCode->empty()) {
                std::cout << "QR Code não encontrado ou vazio no banco, aguardando mais 1.5 segundos e tentando novamente..." << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1500));
                qrCode = db.getQrCodeFromDB(token);
            }

            if (qrCode.has_value() && !qrCode->empty()) {
                std::cout << "QR Code válido encontrado no banco de dados!\n";

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
                std::cout << "Resposta modificada com QR Code do banco: " << response.dump() << std::endl;
            } else {
                std::cerr << "Não foi possível encontrar um QR Code válido no banco de dados para o token: " << token << "\n";
                std::cerr << "O QR Code pode estar vazio ou ainda não ter sido gerado completamente.\n";
            }
        } else {
            std::cout << "QR Code já presente na resposta ou estrutura não reconhecida. Sem necessidade de buscar no banco.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Exceção ao processar resposta: " << e.what() << std::endl;
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
        req_url = std::format("{}/chat/send/image", url);
        req_body = std::format(R"({{"Phone": "{}", "Image": "{}", "Caption" : ""}})", phone, msg_template);
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

Status Wuzapi::createInstance_w(string inst_token, string url, string webhook_url, string proxy_url) {
    Status stat;
    Config cfg;
    Env env = cfg.getEnv();
    Database db;
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
        if (auto connection = db.connect(env.db_url_wuz); connection.status_code == c_status::ERR) {
            std::cout << "ERROR: Erro quando conectando no banco de dados, continuando...\n";
        } else {
            db.insertWebhook_w(inst_token, webhook_url);
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
