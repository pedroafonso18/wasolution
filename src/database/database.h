#pragma once

#include <pqxx/pqxx>
#include "../constants.h"
#include <iostream>
#include <optional>

class Database {
private:
    std::unique_ptr<pqxx::connection> c;
public:
    struct Instance {
        std::string instance_id;
        std::string instance_name;
        std::string instance_type;
        bool is_active;
    };

    Database() = default;
    Status connect(const std::string& db_url);
    std::optional<Instance> fetchInstance(const std::string& instance_id) const;
    Status insertInstance(const std::string& instance_id, const std::string& instance_name, const ApiType& instance_type);
    Status insertLog(const std::string& log_level, const std::string& log_text);
};