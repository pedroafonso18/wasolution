#include "database.h"
#include <sstream>

Status Database::connect(const std::string& db_url) {
    Status stat;
    try {
        c = std::make_unique<pqxx::connection>(db_url);
        if (!c->is_open()) {
            stat.status_code = c_status::ERR;
            stat.status_string = "Failed to open DB connection";
            return stat;
        }
        stat.status_code = c_status::OK;
        stat.status_string = "DB connection opened successfully!";
        return stat;
    } catch (const std::exception& e) {
        stat.status_code = c_status::ERR;
        std::stringstream ss;
        ss << "Connection error: " << e.what();
        stat.status_string = ss.str();
        return stat;
    }
}

std::optional<Database::Instance> Database::fetchInstance(const std::string& instance_id) const {
    Instance inst;
    try {
        if (!c || !c->is_open()) {
            std::cerr << "DB connection is not open, returning nullopt...\n";
            return std::nullopt;
        }
        pqxx::work wrk(*c);
        pqxx::row r = wrk.exec_params1("SELECT instance_id, name, instance_type, is_active FROM instances WHERE instance_id = $1 LIMIT 1", instance_id);
        wrk.commit();
        if (r.empty()) {
            std::cerr << "Couldn't find any instance matching this id..." << std::endl;
            return std::nullopt;
        }
        inst.instance_id = r[0].as<std::string>();
        inst.instance_name = r[1].as<std::string>();
        inst.instance_type = r[2].as<std::string>();
        inst.is_active = r[3].as<bool>();
        return inst;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
    }
}

Status Database::insertInstance(const std::string &instance_id, const std::string &instance_name, const ApiType &instance_type) {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        std::string inst_type;
        if (instance_type == ApiType::EVOLUTION) {
            inst_type = "EVOLUTION";
        } else if (instance_type == ApiType::WUZAPI) {
            inst_type = "WUZAPI";
        }

        pqxx::work wrk(*c);
        pqxx::row r = wrk.exec_params0("INSERT INTO instances (instance_id, name, instance_type, is_active) VALUES ($1, $2, $3, true) RETURNING instance_id", instance_id, instance_name, inst_type);
        wrk.commit();
        if (r.empty()) {
            stat.status_string = "Couldn't insert the instance into the db...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully inserted instance into the db!\n";
        return stat;

    } catch (const std::exception& e) {
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}

Status Database::insertLog(const std::string& log_level, const std::string& log_text) {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        pqxx::row r = wrk.exec_params0("INSERT INTO logs (log_level, log_text) VALUES ($1, $2) RETURNING id", log_level, log_text);
        wrk.commit();
        if (r.empty()) {
            stat.status_string = "Couldn't insert the log into the db...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully inserted log into the db!\n";
        return stat;

    } catch (const std::exception& e) {
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}