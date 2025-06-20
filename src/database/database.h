#pragma once

#include <pqxx/pqxx>
#include "../constants.h"
#include <iostream>
#include <optional>

class Database {
private:
    std::unique_ptr<pqxx::connection> c;
public:
    typedef struct {
        std::string instance_id;
        std::string instance_name;
        std::string instance_type;
        bool is_active;
        std::optional<std::string> webhook_url;
        std::optional<std::string> waba_id;
        std::optional<std::string> access_token;
        std::optional<std::string> phone_number_id;
    } Instance;

    //TODO: Adicionar função para mudar o is_active para true e false.
    Database() = default;
    Status connect(const std::string& db_url);
    std::optional<Instance> fetchInstance(const std::string& instance_id) const;
    Status insertInstance(const std::string& instance_id, const std::string& instance_name, const ApiType& instance_type, std::optional<std::string> webhook_url, std::optional<std::string> waba_id, std::optional<std::string> token, std::optional<std::string> phone_number_id);
    Status insertLog(const std::string& log_level, const std::string& log_text) const;
    Status deleteInstance(const std::string& instance_id);
    std::optional<std::string> getQrCodeFromDB(const std::string& token) const;
    /* Some Wuzapi operations have to be done directly on the database, if someone
       with more knowledge about the api can fix this, I'd be glad to remove any
       api code from the database class.*/
    Status createInstance_w(std::string inst_token, std::string inst_name);
    Status insertWebhook_w(std::string inst_token, std::string webhook_url);
};