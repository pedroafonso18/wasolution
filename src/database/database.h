#pragma once

#include <pqxx/pqxx>
#include "../constants.h"

class Database {
private:
    pqxx::connection c;
    typedef struct {
        std::string instance_id;
        std::string instance_name;
        std::string instance_type;
        bool is_active;
    } Instance;
public:
    pqxx::connection connect(std::string db_url);
    Instance fetchInstance(std::string instance_id);
    Status insertInstance(std::string instance_id, std::string instance_name, ApiType instance_type);
    Status insertLog(std::string log_level, std::string log_text);
};