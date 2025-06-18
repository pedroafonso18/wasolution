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
    try {
        Instance inst;
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

        std::cout << "Iniciando exclusão da instância principal com ID: " << instance_id << std::endl;

        pqxx::work wrk(*c);
        std::string query = "DELETE FROM instances WHERE instance_id = " + wrk.quote(instance_id);

        std::cout << "Executando SQL: " << query << std::endl;
        pqxx::result res = wrk.exec(query);
        std::cout << "Operação DELETE concluída" << std::endl;

        wrk.commit();
        std::cout << "Transação finalizada com sucesso" << std::endl;

        stat.status_code = c_status::OK;
        stat.status_string = "Successfully deleted the instance from the db!\n";
        return stat;

    } catch (const std::exception& e) {
        std::cerr << "Erro na exclusão da instância principal: " << e.what() << std::endl;
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}

Status Database::createInstance_w(std::string inst_token, std::string inst_name) {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "INSERT INTO users (name, token, id) VALUES (" +
            wrk.quote(inst_name) + ", " +
            wrk.quote(inst_token) + ", " +
            wrk.quote(inst_token) + ") RETURNING id"
        );
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

std::optional<std::string> Database::getQrCodeFromDB(const std::string& token) const {
    try {
        if (!c || !c->is_open()) {
            std::cerr << "DB connection is not open, returning nullopt...\n";
            return std::nullopt;
        }

        std::cout << "Procurando QR Code no banco para o token: [" << token << "]" << std::endl;

        pqxx::work wrk(*c);

        std::string sql_query = "SELECT qrcode FROM users WHERE id = " +
                               wrk.quote(token) + " OR token = " +
                               wrk.quote(token) + " LIMIT 1";

        std::cout << "Executando SQL: " << sql_query << std::endl;

        pqxx::result res = wrk.exec(sql_query);
        wrk.commit();

        std::cout << "Consulta executada, número de linhas: " << res.size() << std::endl;

        if (res.empty()) {
            std::cout << "Nenhum registro encontrado no banco para o token fornecido." << std::endl;
            return std::nullopt;
        }

        if (res[0][0].is_null()) {
            std::cout << "QR Code é NULL no banco de dados." << std::endl;
            return std::nullopt;
        }

        std::string qrcode = res[0][0].as<std::string>();
        std::cout << "QR Code encontrado no banco, tamanho: " << qrcode.length() << " caracteres" << std::endl;

        return qrcode;
    } catch (const std::exception& e) {
        std::cerr << "Erro ao recuperar QR Code: " << e.what() << std::endl;
        return std::nullopt;
    }
}

Status Database::deleteInstance_w(std::string inst_token) {
    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        std::cout << "Iniciando operação de exclusão para token: " << inst_token << std::endl;

        pqxx::work wrk(*c);
        std::string query = "DELETE FROM users WHERE id = " +
            wrk.quote(inst_token) + " OR token = " +
            wrk.quote(inst_token);

        std::cout << "Executing SQL query: " << query << std::endl;

        pqxx::result res = wrk.exec(query);

        std::cout << "Query executada, commitando transação" << std::endl;

        wrk.commit();

        std::cout << "Transação finalizada com sucesso" << std::endl;

        stat.status_code = c_status::OK;
        stat.status_string = "Successfully deleted the instance from the db!\n";

        return stat;

    } catch (const std::exception& e) {
        std::cerr << "Erro durante operação de exclusão: " << e.what() << std::endl;
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}
Status Database::insertWebhook_w(std::string inst_token, std::string webhook_url) {

    Status stat;
    try {
        if (!c || !c->is_open()) {
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "UPDATE users SET webhook = " +
            wrk.quote(webhook_url) + " WHERE id = " +
            wrk.quote(inst_token)
        );
        wrk.commit();
        if (res.empty()) {
            stat.status_string = "Couldn't update the webhook on the db...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully updated the webhook on the db!\n";
        return stat;

    } catch (const std::exception& e) {
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}
