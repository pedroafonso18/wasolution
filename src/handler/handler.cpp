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
        stat.status_string = "Couldn't find any connections with this name.\n";
        return stat;
    }
    if (inst.value().instance_type == "EVOLUTION") {
        if (auto snd = Evolution::sendMessage_e(number, env.evo_token, env.evo_url, type, body, instance_name  ); snd.status_code == c_status::ERR) {
            return snd;
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = "Successfully sent the message!";
            return stat;
        }
    } else if (inst.value().instance_type == "WUZAPI") {
        if (auto snd = Wuzapi::sendMessage_w(number, inst.value().instance_id, env.wuz_url, type, body); snd.status_code == c_status::ERR) {
            return snd;
        } else {
            stat.status_code = c_status::OK;
            stat.status_string = "Successfully sent the message!";
            return stat;
        }

    }
    stat.status_code = c_status::ERR;
    stat.status_string = "Unknown API, please choose between EVOLUTION and WUZAPI";
    return stat;
}

Status Handler::createInstance(const string &instance_id, const string &instance_name, ApiType api_type, std::string webhook_url, std::string proxy_url) {
    Config config;
    Database db;
    Status stat;
    Status api_response;

    std::string api_type_s;
    auto env = config.getEnv();
    if (api_type == ApiType::EVOLUTION) {
        api_type_s = "EVOLUTION";
        api_response = Evolution::createInstance_e(env.evo_token, instance_id, instance_name, env.evo_url, webhook_url, proxy_url);
    } else if (api_type == ApiType::WUZAPI) {
        api_type_s = "WUZAPI";
        api_response = Wuzapi::createInstance_w(env.evo_token, instance_id, instance_name, env.wuz_url, webhook_url, proxy_url);
    } else {
        stat.status_code = c_status::ERR;
        stat.status_string = "Unknown API, please choose between EVOLUTION and WUZAPI";
        return stat;
    }
    if (api_response.status_code == c_status::ERR) {
        return api_response;
    }
    if (auto connection = db.connect(env.db_url); connection.status_code == c_status::ERR) {
        return connection;
    }
    auto insertion = db.insertInstance(instance_id, instance_name, api_type);
    if (insertion.status_code == c_status::ERR) {
        return insertion;
    }
    stat.status_code = c_status::OK;
    stat.status_string = "Instance created succesfully!\n";
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
        stat.status_string = "Couldn't get the instance from the db.\n";
        return stat;
    }
    if (instance.value().instance_type == "WUZAPI") {
        return Wuzapi::connectInstance_w(instance_id, env.wuz_url);
    } else if (instance.value().instance_type == "EVOLUTION") {
        return Evolution::connectInstance_e(instance_id, env.evo_url, env.evo_token);
    }
    stat.status_code = c_status::ERR;
    stat.status_string = "Instance type is not valid.\n";
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
        stat.status_string = "Couldn't get the instance from the db.\n";
        return stat;
    }
    if (instance.value().instance_type == "WUZAPI") {
        return Wuzapi::deleteInstance_w(instance_id, env.wuz_token, env.wuz_url);
    } else if (instance.value().instance_type == "EVOLUTION") {
        return Evolution::deleteInstance_e(instance_id, env.evo_token, env.evo_url);
    }
    stat.status_code = c_status::ERR;
    stat.status_string = "Instance type is not valid.\n";
    return stat;
}
