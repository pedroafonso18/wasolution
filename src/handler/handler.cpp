#include "handler.h"

using std::string;

Status Handler::sendMessage(const string &instance_id, string number, string body, MediaType type) {
    Config config;
    Database db;
    std::string instance_name;
    Status stat;

    auto env = config.getEnv();
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }

    auto inst = db.fetchInstance(instance_id);

    if (inst.has_value()) {
        instance_name = inst.value().instance_name;
    } else {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Couldn't find any connections with this name."}};
        return stat;
    }
    if (inst.value().instance_type == "EVOLUTION") {
        if (auto snd = Evolution::sendMessage_e(number, env.evo_token, env.evo_url, type, body, instance_name); snd.status_code == c_status::ERR) {
            return snd;
        } else {
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
        if (auto snd = Wuzapi::sendMessage_w(number, inst.value().instance_id, env.wuz_url, type, body); snd.status_code == c_status::ERR) {
            return snd;
        } else {
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
    stat.status_code = c_status::ERR;
    stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
    return stat;
}

Status Handler::createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url) {
    Config config;
    Database db;
    Status stat;
    Status api_response;

    auto env = config.getEnv();
    std::cout << "EVO_URL: " << env.evo_url << '\n';
    if (api_type == ApiType::EVOLUTION) {
        api_response = Evolution::createInstance_e(env.evo_token, instance_id, instance_name, env.evo_url, webhook_url, proxy_url);
    } else if (api_type == ApiType::WUZAPI) {
        Database db_wuz;
        if (auto connection_wuz = db.connect(env.db_url_wuz); connection_wuz.status_code == c_status::ERR) {
            return connection_wuz;
        }
        if (auto insert_into_wuz = db.createInstance_w(instance_id, instance_name); insert_into_wuz.status_code == c_status::ERR) {
            return insert_into_wuz;
        };
        api_response = Wuzapi::createInstance_w(instance_id, env.wuz_url, webhook_url, proxy_url);
    } else {
        stat.status_code = c_status::ERR;
        stat.status_string = nlohmann::json{{"error", "Unknown API, please choose between EVOLUTION and WUZAPI"}};
        return stat;
    }
    if (api_response.status_code == c_status::ERR) {
        return api_response;
    }
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }
    auto insertion = db.insertInstance(instance_id, instance_name, api_type, webhook_url);
    if (insertion.status_code == c_status::ERR) {
        return insertion;
    }
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
        Database db_wuz;
        Status response = Wuzapi::deleteInstance_w(instance_id, env.wuz_url);
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

        if (response.status_code == c_status::OK) {
            Status dbStatus = db.deleteInstance(instance_id);
            if (dbStatus.status_code == c_status::ERR) {
                std::cerr << "Erro ao excluir instância do banco de dados: "
                          << dbStatus.status_string.dump() << std::endl;
            }
        }
        if (auto connection = db_wuz.connect(env.db_url_wuz); connection.status_code == c_status::ERR) {
            return connection;
        }

        if (auto del = db_wuz.deleteInstance_w(instance_id); del.status_code == c_status::ERR) {
            return del;
        }
        return response;
    } else if (instance.value().instance_type == "EVOLUTION") {
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
        db.deleteInstance(instance_id);
        return response;
    }
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
