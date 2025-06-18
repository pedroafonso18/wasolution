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
    } Instance;

    //TODO: Adicionar função para mudar o is_active para true e false.
    Database() = default;
    Status connect(const std::string& db_url);
    std::optional<Instance> fetchInstance(const std::string& instance_id) const;
    Status insertInstance(const std::string& instance_id, const std::string& instance_name, const ApiType& instance_type, std::optional<std::string> webhook_url);
    Status insertLog(const std::string& log_level, const std::string& log_text) const;
    Status deleteInstance(const std::string& instance_id);
    std::optional<std::string> getQrCodeFromDB(const std::string& token) const;
    /* Wuzapi é uma api bem mal feita, então tem algumas operações que tem de ser feitas
     * diretamente no banco de dados, o que é uma merda, mas fazer o que?*/
    Status createInstance_w(std::string inst_token, std::string inst_name);
    Status insertWebhook_w(std::string inst_token, std::string webhook_url);
};