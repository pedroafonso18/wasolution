#include "cloud_api.h"
#include "logger/logger.h"

using std::string;
extern Logger apiLogger;

// PRIVATE REQUESTS:

Status Cloud::subscribeToWaba_(std::string waba_id, std::string access_token) {
    apiLogger.info("Se inscrevendo na WABA");
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    const string req_url = std::format("https://graph.facebook.com/{}/{}/subscribed_apps", CLOUD_VERSION, waba_id);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Bearer token: {}", access_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

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
            apiLogger.error("Erro HTTP ao se inscrever na waba");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância inscrita com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta ao se inscrever na waba: " + std::string(e.what()));
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

Status Cloud::getPhoneNumberId_(std::string waba_id, std::string access_token) {
        apiLogger.info("Pegando o ID do telefone!");
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    const string req_url = std::format("https://graph.facebook.com/{}/{}/phone_numbers", CLOUD_VERSION, waba_id);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Bearer token: {}", access_token);

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
            apiLogger.error("Erro HTTP ao pegar o id do telefone");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância inscrita com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar resposta ao pegar o id do telefone: " + std::string(e.what()));
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

Status Cloud::registerPhoneNumber_(std::string phone_number_id, std::string access_token) {
            apiLogger.info("Registrando o número na WABA!");
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    const string req_url = std::format("https://graph.facebook.com/{}/{}/register", CLOUD_VERSION, phone_number_id);
    apiLogger.debug("URL da requisição: " + req_url);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Bearer token: {}", access_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

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
            apiLogger.error("Erro HTTP ao registrar o número na WABA");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância registrada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar registrar o número na WABA: " + std::string(e.what()));
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

// PUBLIC REQUESTS

Status Cloud::registerNumber(std::string waba_id, std::string access_token) {
    apiLogger.info("Iniciando registro do número. WABA ID: " + waba_id);
    Status stat;

    apiLogger.debug("Tentando inscrever na WABA...");
    if (auto response = subscribeToWaba_(waba_id, access_token); response.status_code == c_status::ERR) {
        apiLogger.error("Falha ao inscrever na WABA: " + response.status_string.dump());
        return response;
    }
    apiLogger.debug("Inscrição na WABA bem-sucedida. Obtendo ID do telefone...");

    auto number_id = getPhoneNumberId_(waba_id, access_token);
    if (number_id.status_code == c_status::ERR) {
        apiLogger.error("Falha ao obter ID do telefone: " + number_id.status_string.dump());
        return number_id;
    }
    apiLogger.debug("ID do telefone obtido com sucesso: " + number_id.status_string.dump());

    if (number_id.status_string.contains("data") && !number_id.status_string["data"].empty() &&
        !number_id.status_string["data"][0]["id"].empty()) {

        std::string phone_id = number_id.status_string["data"][0]["id"];
        apiLogger.info("ID do telefone encontrado: " + phone_id);

        apiLogger.debug("Registrando o número com o ID: " + phone_id);
        auto rgstr = registerPhoneNumber_(phone_id, access_token);

        if (rgstr.status_code == c_status::OK) {
            apiLogger.info("Número registrado com sucesso!");
        } else {
            apiLogger.error("Falha ao registrar o número: " + rgstr.status_string.dump());
        }

        return rgstr;
    } else {
        apiLogger.error("Dados do ID do telefone não encontrados na resposta");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json {
            {
                "error" , "Couldn't find number_id data."
            }};
        return stat;
    }
}

Status Cloud::sendMessage(std::string instance_id, std::string receiver, std::string body, MediaType m_type, std::string phone_number_id, std::string access_token) {
    apiLogger.info("Enviando mensagem com instância:: " + instance_id);
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }
    string req_body;
    const string req_url = std::format("https://graph.facebook.com/{}/{}/messages", CLOUD_VERSION, phone_number_id );
    if (m_type == MediaType::TEXT) {
    req_body = std::format(R"({{"messaging_product" : "whatsapp","recipient_type" : "individual", "to": "{}", "type" : "text", "text": {{"preview_url" : false, "body" : "{}"}} }})", receiver, body);
    } else if (m_type == MediaType::AUDIO) {
        req_body = std::format(R"({{"messaging_product" : "whatsapp","recipient_type" : "individual", "to": "{}", "type" : "audio", "audio": {{"link" : "{}"}} }})", receiver, body);
    } else if (m_type == MediaType::IMAGE) {
        req_body = std::format(R"({{"messaging_product" : "whatsapp","recipient_type" : "individual", "to": "{}", "type" : "image", "image": {{"link" : "{}"}} }})", receiver, body);
    }
    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Authorization: bearer {}", access_token);

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

Status Cloud::registerTemplate(std::string access_token, Template template_, std::string inst_id, std::string waba_id) {
    apiLogger.info("Registrando o template na instância: " + inst_id);
    CURL *curl = curl_easy_init();
    std::string responseBody;
    Status stat;
    if (!curl) {
        apiLogger.error("Falha ao inicializar CURL");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Failed to initialize CURL"}};
        return stat;
    }

    nlohmann::json request_json = {
        {"name", template_.name},
        {"language", "pt_BR"}
    };

    switch (template_.type) {
        case TemplateType::AUTH:
            request_json["category"] = "AUTHENTICATION";
            break;
        case TemplateType::MARKETING:
            request_json["category"] = "MARKETING";
            break;
        case TemplateType::UTILITY:
            request_json["category"] = "UTILITY";
            break;
        default:
            apiLogger.error("Categoria de template não existente");
            stat.status_code = c_status::ERR;
            stat.status_string = nlohmann::json{{"error", "Failed to create template due to unavailable type"}};
            return stat;
    }

    nlohmann::json components = nlohmann::json::array();

    if (template_.header.header_type != HEADER_T::TEXT) {
        nlohmann::json header_component = {
            {"type", "HEADER"}
        };

        switch (template_.header.header_type) {
            case HEADER_T::IMAGE:
                header_component["format"] = "IMAGE";
                break;
            case HEADER_T::DOCUMENT:
                header_component["format"] = "DOCUMENT";
                break;
            case HEADER_T::LOCATION:
                header_component["format"] = "LOCATION";
                break;
            default:
                break;
        }

        components.push_back(header_component);
    } else if (!template_.header.header_content.empty()) {
        nlohmann::json header_component = {
            {"type", "HEADER"},
            {"format", "TEXT"},
            {"text", template_.header.header_content}
        };

        components.push_back(header_component);
    }

    if (!template_.BODY.text.empty()) {
        nlohmann::json body_component = {
            {"type", "BODY"},
            {"text", template_.BODY.text}
        };

        if (!template_.BODY.examples.empty()) {
            nlohmann::json example_array = nlohmann::json::array();
            example_array.push_back(template_.BODY.examples);

            body_component["example"] = {
                {"body_text", example_array}
            };
        }

        components.push_back(body_component);
    }

    if (!template_.FOOTER.empty()) {
        nlohmann::json footer_component = {
            {"type", "FOOTER"},
            {"text", template_.FOOTER}
        };

        components.push_back(footer_component);
    }

    if (!template_.BUTTONS.empty()) {
        nlohmann::json buttons_component = {
            {"type", "BUTTONS"},
            {"buttons", nlohmann::json::array()}
        };

        for (const auto& button : template_.BUTTONS) {
            buttons_component["buttons"].push_back({
                {"type", button.type},
                {"text", button.text}
            });
        }

        components.push_back(buttons_component);
    }

    request_json["components"] = components;

    string req_body = request_json.dump();
    const string req_url = std::format("https://graph.facebook.com/{}/{}/message_templates", CLOUD_VERSION, waba_id);

    apiLogger.debug("URL da requisição: " + req_url);
    apiLogger.debug("Corpo da requisição: " + req_body);

    struct curl_slist *headers = nullptr;
    const string authorization = std::format("Bearer token: {}", access_token);

    headers = curl_slist_append(headers, authorization.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "accept: application/json");

    curl_easy_setopt(curl, CURLOPT_URL, req_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &responseBody);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "POST");

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
            apiLogger.error("Erro HTTP ao registrar o número na WABA");
            stat.status_code = c_status::ERR;
            stat.status_string = response;
            if (!response.contains("error")) {
                response["error"] = "Servidor retornou código de erro HTTP";
                stat.status_string = response;
            }
        } else {
            apiLogger.info("Instância registrada com sucesso");
            stat.status_code = c_status::OK;
            stat.status_string = response;
        }
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao processar registrar o número na WABA: " + std::string(e.what()));
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
