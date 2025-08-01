#include "handler.h"

#include "cloud/cloud_api.h"
#include "logger/logger.h"

using std::string;

extern Logger apiLogger;

Status Handler::sendMessage(const string &instance_id, string number, string body, MediaType type) {
    apiLogger.info("Iniciando envio de mensagem para instância: " + instance_id);
    Config config;
    Database db;
    Database evo_db;
    std::string instance_name;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco de dados: " + connection.status_string.dump());
        return connection;
    }

    auto inst = db.fetchInstance(instance_id);

    if (inst.has_value()) {
        instance_name = inst.value().instance_name;
        apiLogger.debug("Instância encontrada: " + instance_name);
    } else {
        apiLogger.error("Instância não encontrada: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't find any connections with this name."}};
        return stat;
    }

    ApiType api_type;
    if (inst.value().instance_type == "EVOLUTION") {
        api_type = ApiType::EVOLUTION;
        if (env.db_url_evo.empty()) {
            apiLogger.warn("URL do banco de dados Evolution não configurada");
        } else if (auto evo_connection = evo_db.connect(env.db_url_evo); evo_connection.status_code == c_status::ERR) {
            apiLogger.warn("Erro ao conectar ao banco de dados Evolution: " + evo_connection.status_string.dump());
        }
    } else if (inst.value().instance_type == "WUZAPI") {
        api_type = ApiType::WUZAPI;
    } else if (inst.value().instance_type == "CLOUD") {
        api_type = ApiType::CLOUD;
    } else {
        apiLogger.error("Tipo de instância desconhecido: " + inst.value().instance_type);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown instance type"}};
        return stat;
    }

    bool is_active = db.isActive(api_type, instance_id, evo_db);
    if (!is_active) {
        apiLogger.error("Instância não está ativa: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance is not active. Please connect it first."}};
        return stat;
    }

    Status snd;

    if (inst.value().instance_type == "EVOLUTION") {
        apiLogger.info("Enviando mensagem via Evolution API");
        snd = Evolution::sendMessage_e(number, env.evo_token, env.evo_url, type, body, instance_name);
        if (snd.status_code == c_status::ERR) {
            apiLogger.error("Erro ao enviar mensagem via Evolution: " + snd.status_string.dump());
            return snd;
        } else {
            apiLogger.info("Mensagem enviada com sucesso via Evolution");
            stat.status_code = c_status::OK;
            try {
                stat.status_string = nlohmann::json::parse(snd.status_string.dump());
            } catch (const std::exception& e) {
                stat.status_string = nlohmann::json{
                    {"message", "Successfully sent the message!"},
                    {"api_response", snd.status_string}
                };
            }
            return stat;
        }
    } else if (inst.value().instance_type == "WUZAPI") {
        apiLogger.info("Enviando mensagem via WuzAPI");
        snd = Wuzapi::sendMessage_w(number, inst.value().instance_id, env.wuz_url, type, body);
        if (snd.status_code == c_status::ERR) {
            apiLogger.error("Erro ao enviar mensagem via WuzAPI: " + snd.status_string.dump());
            return snd;
        } else {
            apiLogger.info("Mensagem enviada com sucesso via WuzAPI");
            stat.status_code = c_status::OK;
            try {
                stat.status_string = nlohmann::json::parse(snd.status_string.dump());
            } catch (const std::exception& e) {
                stat.status_string = nlohmann::json{
                    {"message", "Successfully sent the message!"},
                    {"api_response", snd.status_string}
                };
            }
            return stat;
        }
    } else if (inst.value().instance_type == "CLOUD") {
        apiLogger.info("Enviando mensagem via CLOUD");
        snd = Cloud::sendMessage(instance_id, number, body, type, inst.value().phone_number_id.value(), inst.value().access_token.value());
        if (snd.status_code == c_status::ERR) {
            apiLogger.error("Erro ao enviar mensagem via CLOUD: " + snd.status_string.dump());
            return snd;
        } else {
            apiLogger.info("Mensagem enviada com sucesso via CLOUD");
            stat.status_code = c_status::OK;
            try {
                stat.status_string = nlohmann::json::parse(snd.status_string.dump());
            } catch (const std::exception& e) {
                stat.status_string = nlohmann::json{
                        {"message", "Successfully sent the message!"},
                        {"api_response", snd.status_string}
                };
            }
            return stat;
        }
    }
    apiLogger.error("Tipo de API desconhecido para instância: " + instance_id);
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
    return stat;
}

Status Handler::createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url, std::string access_token, std::string waba_id) {
    apiLogger.info("Iniciando criação de instância: " + instance_id + " (" + instance_name + ")");
    Config config;
    Database db;
    Status stat;
    Status api_response;

    auto env = config.getEnv();
    if (api_type == ApiType::EVOLUTION) {
        apiLogger.info("Criando instância Evolution");
        api_response = Evolution::createInstance_e(env.evo_token, instance_id, instance_name, env.evo_url, webhook_url, proxy_url);
    } else if (api_type == ApiType::WUZAPI) {
        apiLogger.info("Criando instância WuzAPI");
        api_response = Wuzapi::createInstance_w(instance_id, instance_name, env.wuz_url, webhook_url, proxy_url, env.wuz_admin_token);
    } else if (api_type == ApiType::CLOUD) {
        apiLogger.info("Criando instância Cloud");
        api_response = Cloud::registerNumber(waba_id, access_token);
    }
    else {
        apiLogger.error("Tipo de API desconhecido");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
        return stat;
    }
    if (api_response.status_code == c_status::ERR) {
        apiLogger.error("Erro na criação da instância: " + api_response.status_string.dump());
        return api_response;
    }

    /*if (api_type == ApiType::EVOLUTION && !env.rabbit_url.empty()) {
        apiLogger.info("Configurando RabbitMQ para instância Evolution: " + instance_name);
        Status rabbit_response = Evolution::setRabbit_e(instance_name, env.rabbit_url, env.evo_url, env.evo_token);
        if (rabbit_response.status_code == c_status::ERR) {
            apiLogger.warn("Falha ao configurar RabbitMQ para instância: " + instance_name + " - " + rabbit_response.status_string.dump());
        } else {
            apiLogger.info("RabbitMQ configurado com sucesso para instância: " + instance_name);
        }
    } */

    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco principal: " + connection.status_string.dump());
        return connection;
    }
    std::string phone_number_id;
    if (api_response.status_string.contains("id") && api_response.status_string.contains("data") && !api_response.status_string["data"].empty() &&
        !api_response.status_string["data"][0]["id"].empty()) {
        phone_number_id = api_response.status_string["data"][0]["id"];
    } else {
        phone_number_id = "";
    }
    auto insertion = db.insertInstance(instance_id, instance_name, api_type, webhook_url, waba_id, access_token, phone_number_id);
    if (insertion.status_code == c_status::ERR) {
        apiLogger.error("Erro ao inserir instância no banco principal: " + insertion.status_string.dump());
        return insertion;
    }
    apiLogger.info("Instância criada com sucesso: " + instance_id);
    stat.status_code = c_status::OK;

    try {
        nlohmann::json response = nlohmann::json::parse(api_response.status_string.dump());
        response["message"] = "Instance created successfully!";
        stat.status_string = response;
    } catch (const std::exception& e) {
        stat.status_string = nlohmann::json{
            {"message", "Instance created successfully!"},
            {"api_response", api_response.status_string}
        };
    }

    return stat;
}

Status Handler::connectInstance(string instance_id) {
    Config config;
    Database db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }

    auto instance = db.fetchInstance(instance_id);
    if (!instance.has_value()) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
        return stat;
    }
    if (instance.value().instance_type == "WUZAPI") {
        Status response = Wuzapi::connectInstance_w(instance_id, env.wuz_url);
        if (response.status_code == c_status::ERR) {
            return response;
        }

        apiLogger.info("Instance connected successfully, fetching QR code");
        Status qrResponse = Wuzapi::getQrCode_w(instance_id, env.wuz_url);

        try {
            if (qrResponse.status_code == c_status::OK) {
                nlohmann::json combinedResponse = nlohmann::json::parse(qrResponse.status_string.dump());
                combinedResponse["message"] = "Instance connected successfully!";
                combinedResponse["connection_status"] = nlohmann::json::parse(response.status_string.dump());
                qrResponse.status_string = combinedResponse;
            }
        } catch (const std::exception& e) {
            if (qrResponse.status_code == c_status::OK) {
                qrResponse.status_string = nlohmann::json{
                    {"message", "Instance connected successfully!"},
                    {"api_response", qrResponse.status_string},
                    {"connection_status", response.status_string}
                };
            }
        }

        return qrResponse;
    } else if (instance.value().instance_type == "EVOLUTION") {
        Status response = Evolution::connectInstance_e(instance.value().instance_name, env.evo_url, env.evo_token);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                    {"message", "Instance connected successfully!"},
                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;
}

Status Handler::deleteInstance(string instance_id) {
    apiLogger.info("Iniciando exclusão da instância: " + instance_id);
    Config config;
    Database db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco: " + connection.status_string.dump());
        return connection;
    }

    auto instance = db.fetchInstance(instance_id);
    if (!instance.has_value()) {
        apiLogger.error("Instância não encontrada: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
        return stat;
    }

    Status dbStatus = db.deleteInstance(instance_id);
    if (dbStatus.status_code == c_status::ERR) {
        apiLogger.error("Erro ao excluir instância do banco: " + dbStatus.status_string.dump());
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't delete the instance from the db..."}};
        return stat;
    }

    if (instance.value().instance_type == "WUZAPI") {
        apiLogger.info("Excluindo instância WUZAPI");
        Status response = Wuzapi::deleteInstance_w(instance_id,env.wuz_url, env.wuz_admin_token);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                        {"message", "Instance deleted successfully!"},
                        {"api_response", response.status_string}
                };
            }
        }
        return response;

    } else if (instance.value().instance_type == "EVOLUTION") {
        apiLogger.info("Excluindo instância Evolution");
        Status response = Evolution::deleteInstance_e(instance_id, env.evo_token, env.evo_url);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                    {"message", "Instance deleted successfully!"},
                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    apiLogger.error("Tipo de instância inválido: " + instance_id);
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;
}


Status Handler::logoutInstance(string instance_id) {
    Config config;
    Database db;
    Database evo_db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco de dados: " + connection.status_string.dump());
        return connection;
    }

    auto instance = db.fetchInstance(instance_id);
    if (!instance.has_value()) {
        apiLogger.error("Instância não encontrada: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
        return stat;
    }

    ApiType api_type;
    if (instance.value().instance_type == "EVOLUTION") {
        api_type = ApiType::EVOLUTION;
        if (env.db_url_evo.empty()) {
            apiLogger.warn("URL do banco de dados Evolution não configurada");
        } else if (auto evo_connection = evo_db.connect(env.db_url_evo); evo_connection.status_code == c_status::ERR) {
            apiLogger.warn("Erro ao conectar ao banco de dados Evolution: " + evo_connection.status_string.dump());
        }
    } else if (instance.value().instance_type == "WUZAPI") {
        api_type = ApiType::WUZAPI;
    } else if (instance.value().instance_type == "CLOUD") {
        api_type = ApiType::CLOUD;
    } else {
        apiLogger.error("Tipo de instância desconhecido: " + instance.value().instance_type);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown instance type"}};
        return stat;
    }

    bool is_active = db.isActive(api_type, instance_id, evo_db);
    if (!is_active) {
        apiLogger.error("Instância não está ativa: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance is not active. Please connect it first."}};
        return stat;
    }

    if (instance.value().instance_type == "WUZAPI") {
        Status response = Wuzapi::logoutInstance_w(instance_id, env.wuz_url);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                    {"message", "Instance deleted successfully!"},
                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    } else if (instance.value().instance_type == "EVOLUTION") {
        Status response = Evolution::logoutInstance_e(instance_id, env.evo_url, env.evo_token);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                    {"message", "Instance deleted successfully!"},
                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;
}

Status Handler::setWebhook(string token, string webhook_url) {
    Config config;
    Database db;
    Database evo_db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco de dados: " + connection.status_string.dump());
        return connection;
    }

    auto instance = db.fetchInstance(token);
    if (!instance.has_value()) {
        apiLogger.error("Instância não encontrada: " + token);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
        return stat;
    }

    ApiType api_type;
    if (instance.value().instance_type == "EVOLUTION") {
        api_type = ApiType::EVOLUTION;
        if (env.db_url_evo.empty()) {
            apiLogger.warn("URL do banco de dados Evolution não configurada");
        } else if (auto evo_connection = evo_db.connect(env.db_url_evo); evo_connection.status_code == c_status::ERR) {
            apiLogger.warn("Erro ao conectar ao banco de dados Evolution: " + evo_connection.status_string.dump());
        }
    } else if (instance.value().instance_type == "WUZAPI") {
        api_type = ApiType::WUZAPI;
    } else if (instance.value().instance_type == "CLOUD") {
        api_type = ApiType::CLOUD;
    } else {
        apiLogger.error("Tipo de instância desconhecido: " + instance.value().instance_type);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown instance type"}};
        return stat;
    }

    bool is_active = db.isActive(api_type, token, evo_db);
    if (!is_active) {
        apiLogger.error("Instância não está ativa: " + token);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance is not active. Please connect it first."}};
        return stat;
    }

    if (instance.value().instance_type == "WUZAPI") {
        Status response = Wuzapi::setWebhook_w(token, webhook_url, env.wuz_url);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                        {"message", "Instance deleted successfully!"},
                        {"api_response", response.status_string}
                };
            }
        }
        return response;
    } else if (instance.value().instance_type == "EVOLUTION") {
        Status response = Evolution::setWebhook_e(instance.value().instance_name, webhook_url, env.evo_url, env.evo_token);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                        {"message", "Instance deleted successfully!"},
                        {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;
}

std::vector<Database::Instance> Handler::retrieveInstances() {
    Database db;
    Database evo_db;
    Config cfg;
    auto env = cfg.getEnv();

    apiLogger.info("Retrieving all instances from database");

    std::vector<Database::Instance> instances;
    auto connection = db.connect(env.db_url);
    if (connection.status_code == c_status::ERR) {
        apiLogger.error("Failed to connect to database: " + connection.status_string.dump());
        return instances;
    }

    bool evo_db_connected = false;
    if (!env.db_url_evo.empty()) {
        auto evo_connection = evo_db.connect(env.db_url_evo);
        if (evo_connection.status_code == c_status::OK) {
            evo_db_connected = true;
            apiLogger.debug("Connected to Evolution database for instance status verification");
        } else {
            apiLogger.warn("Failed to connect to Evolution database: " + evo_connection.status_string.dump());
        }
    }

    instances = db.retrieveInstances();
    apiLogger.info("Retrieved " + std::to_string(instances.size()) + " instances");

    for (auto& instance : instances) {
        ApiType api_type;
        if (instance.instance_type == "EVOLUTION") {
            api_type = ApiType::EVOLUTION;
        } else if (instance.instance_type == "WUZAPI") {
            api_type = ApiType::WUZAPI;
        } else if (instance.instance_type == "CLOUD") {
            api_type = ApiType::CLOUD;
        } else {
            apiLogger.warn("Unknown instance type for instance " + instance.instance_id + ": " + instance.instance_type);
            continue;
        }

        if (instance.instance_type == "EVOLUTION" && !evo_db_connected) {
            apiLogger.debug("Skipping activity verification for Evolution instance " + instance.instance_id + " - Evolution DB not connected");
            continue;
        }

        bool is_active = db.isActive(api_type, instance.instance_id, evo_db);
        instance.is_active = is_active;

        apiLogger.debug("Instance " + instance.instance_id + " (" + instance.instance_type + ") activity status: " +
                       (is_active ? "active" : "inactive"));
    }

    return instances;
}

 Status Handler::sendTemplate(string instance_id, string number, string body, MediaType type, std::vector<FB_VARS> vars, std::string template_name) {
    Database db;
    Config cfg;
    Status stat;
    auto env = cfg.getEnv();

    apiLogger.info("Sending template from instance: " + instance_id + " - Template: " + template_name);

    auto connection = db.connect(env.db_url);
    if (connection.status_code == c_status::ERR) {
        apiLogger.error("Failed to connect to database: " + connection.status_string.dump());
        return connection;
    }

    auto instance_opt = db.fetchInstance(instance_id);
    if (!instance_opt.has_value()) {
        apiLogger.error("Instance not found: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance not found"}};
        return stat;
    }

    auto instance = instance_opt.value();
    if (instance.instance_type != "CLOUD") {
        apiLogger.error("Instance type not compatible, should be CLOUD. Current: " + instance.instance_type);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance type not compatible, should be CLOUD"}};
        return stat;
    }

    if (!instance.phone_number_id.has_value() || !instance.access_token.has_value()) {
        apiLogger.error("Missing required fields for Cloud API (phone_number_id or access_token)");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Missing required Cloud API configuration"}};
        return stat;
    }

    apiLogger.info("Sending template via Cloud API");
    return Cloud::sendTemplate(
        instance_id,
        number,
        body,
        type,
        instance.phone_number_id.value(),
        instance.access_token.value(),
        vars,
        template_name
    );
}

Status Handler::createGroup(string instance_id, string subject, string description, std::vector<string> participants) {
    Config config;
    Database db;
    Database evo_db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco de dados: " + connection.status_string.dump());
        return connection;
    }

    auto instance = db.fetchInstance(instance_id);
    if (!instance.has_value()) {
        apiLogger.error("Instância não encontrada: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
        return stat;
    }

    ApiType api_type;
    if (instance.value().instance_type == "EVOLUTION") {
        api_type = ApiType::EVOLUTION;
        if (env.db_url_evo.empty()) {
            apiLogger.warn("URL do banco de dados Evolution não configurada");
        } else if (auto evo_connection = evo_db.connect(env.db_url_evo); evo_connection.status_code == c_status::ERR) {
            apiLogger.warn("Erro ao conectar ao banco de dados Evolution: " + evo_connection.status_string.dump());
        }
    } else if (instance.value().instance_type == "WUZAPI") {
        api_type = ApiType::WUZAPI;
    } else if (instance.value().instance_type == "CLOUD") {
        api_type = ApiType::CLOUD;
    } else {
        apiLogger.error("Tipo de instância desconhecido: " + instance.value().instance_type);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown instance type"}};
        return stat;
    }

    bool is_active = db.isActive(api_type, instance_id, evo_db);
    if (!is_active) {
        apiLogger.error("Instância não está ativa: " + instance_id);
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Instance is not active. Please connect it first."}};
        return stat;
    }

    if (instance.value().instance_type == "WUZAPI") {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{ { "error", "Still not implemented for Wuzapi"}};
    } else if (instance.value().instance_type == "EVOLUTION") {
        Status response = Evolution::createGroup_e(env.evo_token, env.evo_url, instance.value().instance_name, subject, description, participants);
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                            {"message", "Group created successfully!"},
                            {"api_response", response.status_string}
                };
            }
        }
        return response;
    }
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Instance type is not valid."}};
    return stat;


}
