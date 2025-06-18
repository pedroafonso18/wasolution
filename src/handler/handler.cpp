#include "handler.h"
#include "logger/logger.h"

using std::string;

extern Logger apiLogger;

Status Handler::sendMessage(const string &instance_id, string number, string body, MediaType type) {
    apiLogger.info("Iniciando envio de mensagem para instância: " + instance_id);
    Config config;
    Database db;
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
    }
    apiLogger.error("Tipo de API desconhecido para instância: " + instance_id);
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
    return stat;
}

Status Handler::createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url) {
    apiLogger.info("Iniciando criação de instância: " + instance_id + " (" + instance_name + ")");
    Config config;
    Database db;
    Status stat;
    Status api_response;

    auto env = config.getEnv();
    std::string web_url = std::format("http://{}:{}/webhook", env.ip_address, PORT);
    if (api_type == ApiType::EVOLUTION) {
        apiLogger.info("Criando instância Evolution");
        api_response = Evolution::createInstance_e(env.evo_token, instance_id, instance_name, env.evo_url, web_url, proxy_url);
    } else if (api_type == ApiType::WUZAPI) {
        apiLogger.info("Criando instância WuzAPI");
        api_response = Wuzapi::createInstance_w(instance_id, instance_name, env.wuz_url, web_url, proxy_url, env.wuz_admin_token);
    } else {
        apiLogger.error("Tipo de API desconhecido");
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
        return stat;
    }
    if (api_response.status_code == c_status::ERR) {
        apiLogger.error("Erro na criação da instância: " + api_response.status_string.dump());
        return api_response;
    }
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        apiLogger.error("Erro ao conectar ao banco principal: " + connection.status_string.dump());
        return connection;
    }
    auto insertion = db.insertInstance(instance_id, instance_name, api_type, webhook_url);
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

Status Handler::sendWebhook(nlohmann::json webhook, string inst_id) {
    Config config;
    Database db;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }

    if (auto instance = db.fetchInstance(inst_id); instance.value().instance_type == "WUZAPI") {
        Status response = Webhook::deserialize_webhook(webhook, ApiType::WUZAPI, instance.value().webhook_url.value());
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                                { "webhook", "Webhook sent successfully!" },
                                    {"api_response", response.status_string}
                };
            }
        }
        return response;
    } else if (instance.value().instance_type == "EVOLUTION") {
        Status response = Webhook::deserialize_webhook(webhook, ApiType::EVOLUTION, instance.value().webhook_url.value());
        try {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json::parse(response.status_string.dump());
            }
        } catch (const std::exception& e) {
            if (response.status_code == c_status::OK) {
                response.status_string = nlohmann::json{
                            {"webhook", "Webhook sent successfully!"},
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


Status Handler::logoutInstance(string instance_id) {
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
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }

    auto instance = db.fetchInstance(token);
    if (!instance.has_value()) {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't get the instance from the db."}};
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
