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
        pqxx::result res = wrk.exec(
            "SELECT instance_id, name, instance_type, is_active, webhook_url FROM instances WHERE instance_id = " +
            wrk.quote(instance_id) + " LIMIT 1"
        );
        wrk.commit();
        if (res.empty()) {
            return std::nullopt;
        }
        const pqxx::row& r = res[0];
        inst.instance_id = r[0].as<std::string>();
        inst.instance_name = r[1].as<std::string>();
        inst.instance_type = r[2].as<std::string>();
        inst.is_active = r[3].as<bool>();
        inst.webhook_url = r[4].as<std::string>();
        return inst;
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return std::nullopt;
    }
}

Status Database::insertInstance(const std::string &instance_id, const std::string &instance_name, const ApiType &instance_type, std::optional<std::string> webhook_url) {
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
        pqxx::result res;
        pqxx::work wrk(*c);
        if (!webhook_url.has_value()) {
            res = wrk.exec(
                "INSERT INTO instances (instance_id, name, instance_type, is_active) VALUES (" +
                wrk.quote(instance_id) + ", " +
                wrk.quote(instance_name) + ", " +
                wrk.quote(inst_type) + ", true) RETURNING instance_id"
            );
        } else if (webhook_url.has_value()) {
            res = wrk.exec(
                "INSERT INTO instances (instance_id, name, webhook_url, instance_type, is_active) VALUES (" +
                wrk.quote(instance_id) + ", " +
                wrk.quote(instance_name) + ", " +
                wrk.quote(webhook_url.value()) + ", " +
                wrk.quote(inst_type) + ", true) RETURNING instance_id"
            );
        }
        wrk.commit();
        if (res.empty()) {
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

Status Database::insertLog(const std::string& log_level, const std::string& log_text) const {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "INSERT INTO logs (log_level, log_text) VALUES (" +
            wrk.quote(log_level) + ", " +
            wrk.quote(log_text) + ") RETURNING id"
        );
        wrk.commit();
        if (res.empty()) {
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

Status Database::deleteInstance(const std::string &instance_id) {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "DELETE FROM instances WHERE instance_id = " +
            wrk.quote(instance_id)
        );
        wrk.commit();
        if (res.empty()) {
            stat.status_string = "Couldn't delete the instance from the db...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully deleted the instance from the db!\n";
        return stat;

    } catch (const std::exception& e) {
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}
