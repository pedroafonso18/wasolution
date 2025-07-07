#include "database.h"
#include "logger/logger.h"
#include <sstream>

extern Logger apiLogger;

Status Database::connect(const std::string& db_url) {
    apiLogger.debug("Tentando conectar ao banco de dados");
    Status stat;
    try {
        c = std::make_unique<pqxx::connection>(db_url);
        if (!c->is_open()) {
            apiLogger.error("Falha ao abrir conexão com o banco de dados");
            stat.status_code = c_status::ERR;
            stat.status_string = "Failed to open DB connection";
            return stat;
        }
        apiLogger.info("Conexão com banco de dados estabelecida com sucesso");
        stat.status_code = c_status::OK;
        stat.status_string = "DB connection opened successfully!";
        return stat;
    } catch (const std::exception& e) {
        apiLogger.error("Erro na conexão com banco de dados: " + std::string(e.what()));
        stat.status_code = c_status::ERR;
        std::stringstream ss;
        ss << "Connection error: " << e.what();
        stat.status_string = ss.str();
        return stat;
    }
}

std::optional<Database::Instance> Database::fetchInstance(const std::string& instance_id) const {
    apiLogger.debug("Buscando instância: " + instance_id);
    try {
        Instance inst;
        if (!c || !c->is_open()) {
            apiLogger.error("Conexão com banco de dados não está aberta");
            return std::nullopt;
        }
        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "SELECT instance_id, name, instance_type, is_active, webhook_url, waba_id, access_token FROM instances WHERE instance_id = " +
            wrk.quote(instance_id) + " LIMIT 1"
        );
        wrk.commit();
        if (res.empty()) {
            apiLogger.debug("Instância não encontrada: " + instance_id);
            return std::nullopt;
        }
        const pqxx::row& r = res[0];
        inst.instance_id = r[0].as<std::string>();
        inst.instance_name = r[1].as<std::string>();
        inst.instance_type = r[2].as<std::string>();
        inst.is_active = r[3].as<bool>();
        inst.webhook_url = r[4].as<std::string>();
        inst.waba_id = r[5].as<std::string>();
        inst.access_token = r[6].as<std::string>();
        apiLogger.debug("Instância encontrada: " + inst.instance_name);
        return inst;
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao buscar instância: " + std::string(e.what()));
        return std::nullopt;
    }
}

Status Database::insertInstance(const std::string &instance_id, const std::string &instance_name, const ApiType &instance_type, std::optional<std::string> webhook_url, std::optional<std::string> waba_id, std::optional<std::string> token, std::optional<std::string> phone_number_id) {
    apiLogger.info("Inserindo nova instância: " + instance_id + " (" + instance_name + ")");
    Status stat;
    try {
        if (!c || !c->is_open()) {
            apiLogger.error("Conexão com banco de dados não está aberta");
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        std::string inst_type;
        if (instance_type == ApiType::EVOLUTION) {
            inst_type = "EVOLUTION";
        } else if (instance_type == ApiType::WUZAPI) {
            inst_type = "WUZAPI";
        } else if (instance_type == ApiType::CLOUD) {
            inst_type = "CLOUD";
        }
        pqxx::result res;
        pqxx::work wrk(*c);

        std::string columns = "instance_id, name, instance_type, is_active";
        std::string values = wrk.quote(instance_id) + ", " +
                             wrk.quote(instance_name) + ", " +
                             wrk.quote(inst_type) + ", true";

        if (webhook_url.has_value()) {
            columns += ", webhook_url";
            values += ", " + wrk.quote(webhook_url.value());
        }

        if (waba_id.has_value()) {
            columns += ", waba_id";
            values += ", " + wrk.quote(waba_id.value());
            apiLogger.debug("Incluindo WABA ID na inserção: " + waba_id.value());
        }

        if (token.has_value()) {
            columns += ", access_token";
            values += ", " + wrk.quote(token.value());
            apiLogger.debug("Incluindo ACCESS_TOKEN na inserção: " + (token.has_value() ? token.value() : "null"));
        }

        if (phone_number_id.has_value()) {
            columns += ", phone_number_id";
            values += ", " + wrk.quote(phone_number_id.value());
            apiLogger.debug("Incluindo PHONE_NUMBER_ID na inserção: " + phone_number_id.value());
        }

        res = wrk.exec(
            "INSERT INTO instances (" + columns + ") VALUES (" + values + ") RETURNING instance_id"
        );

        wrk.commit();
        if (res.empty()) {
            apiLogger.error("Falha ao inserir instância no banco de dados");
            stat.status_string = "Couldn't insert the instance into the db...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }
        apiLogger.info("Instância inserida com sucesso: " + instance_id);
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully inserted instance into the db!\n";
        return stat;

    } catch (const std::exception& e) {
        apiLogger.error("Erro ao inserir instância: " + std::string(e.what()));
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

Status Database::deleteInstance(const std::string &instance_id) {
    apiLogger.info("Iniciando exclusão da instância: " + instance_id);
    Status stat;
    try {
        if (!c || !c->is_open()) {
            apiLogger.error("Conexão com banco de dados não está aberta");
            stat.status_string = "DB connection is not open, returning error...\n";
            stat.status_code = c_status::ERR;
            return stat;
        }

        pqxx::work wrk(*c);
        std::string query = "DELETE FROM instances WHERE instance_id = " + wrk.quote(instance_id);
        apiLogger.debug("Executando query: " + query);

        pqxx::result res = wrk.exec(query);
        wrk.commit();

        apiLogger.info("Instância excluída com sucesso: " + instance_id);
        stat.status_code = c_status::OK;
        stat.status_string = "Successfully deleted the instance from the db!\n";
        return stat;

    } catch (const std::exception& e) {
        apiLogger.error("Erro ao excluir instância: " + std::string(e.what()));
        stat.status_string = e.what();
        stat.status_code = c_status::ERR;
        return stat;
    }
}

std::vector<Database::Instance> Database::retrieveInstances() {
    apiLogger.debug("Buscando instâncias.");
    std::vector<Instance> instVec;
    try {
        if (!c || !c->is_open()) {
            apiLogger.error("Conexão com banco de dados não está aberta");
            return instVec;
        }
        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "SELECT instance_id, name, instance_type, is_active, webhook_url, waba_id, access_token, phone_number_id FROM instances"
        );
        wrk.commit();
        if (res.empty()) {
            apiLogger.debug("Nenhuma instância encontrada.");
            return instVec;
        }

        for (const auto& row : res) {
            Instance inst;
            inst.instance_id = row[0].as<std::string>();
            inst.instance_name = row[1].as<std::string>();
            inst.instance_type = row[2].as<std::string>();
            inst.is_active = row[3].as<bool>();

            if (!row[4].is_null()) {
                inst.webhook_url = row[4].as<std::string>();
            }

            if (!row[5].is_null()) {
                inst.waba_id = row[5].as<std::string>();
            }

            if (!row[6].is_null()) {
                inst.access_token = row[6].as<std::string>();
            }

            if (!row[7].is_null()) {
                inst.phone_number_id = row[7].as<std::string>();
            }

            apiLogger.debug("Instância encontrada: " + inst.instance_name + " (ID: " + inst.instance_id + ")");
            instVec.push_back(inst);
        }

        apiLogger.info("Recuperadas " + std::to_string(instVec.size()) + " instâncias do banco de dados");
        return instVec;
    } catch (const std::exception& e) {
        apiLogger.error("Erro ao buscar instâncias: " + std::string(e.what()));
        return instVec;
    }
}

std::unique_ptr<pqxx::connection> *Database::getConn() {
    return &c;
}

bool Database::fetchIsActive_e(std::string inst_id, Database& db) {
    apiLogger.debug("Buscando instância dentro da evolution...");
    try {
        auto* conn = db.getConn();
        if (!conn || !(*conn) || !(*conn)->is_open()) {
            apiLogger.error("Conexão com banco de dados não está aberta");
            return false;
        }

        pqxx::work wrk(*(*conn));
        pqxx::result res;
        res = wrk.exec(
            "SELECT \"connectionStatus\" FROM \"Instance\" WHERE token = " + wrk.quote(inst_id)
        );
        wrk.commit();

        if (res.empty()) {
            apiLogger.debug("Nenhuma instância encontrada no Evolution database para: " + inst_id);
            return false;
        }

        std::string resp = res[0][0].as<std::string>();
        apiLogger.debug("Estado da instância no Evolution: " + resp);

        if (resp == "open" || resp == "connecting") {
            return true;
        } else {
            return false;
        }

    } catch (const std::exception& e) {
        apiLogger.error("Erro ao buscar instância no Evolution DB: " + std::string(e.what()));
        return false;
    }
}

bool Database::isActive(const ApiType &instance_type, std::string inst_id, Database& db) {
    apiLogger.debug("Verificando se instância está ativa: " + inst_id);
    bool is_active = false;
    if (instance_type == ApiType::EVOLUTION) {
        is_active = fetchIsActive_e(inst_id, db);
    } else if (instance_type == ApiType::CLOUD) {
        is_active = true;
    } else if (instance_type == ApiType::WUZAPI) {
        is_active = true;
    }

    try {
        if (!c || !c->is_open()) {
            apiLogger.error("Conexão com o banco de dados não está aberta, retornando is_active sem atualizar...");
            return is_active;
        }

        pqxx::work wrk(*c);
        pqxx::result res = wrk.exec(
            "SELECT is_active FROM instances WHERE instance_id = " + wrk.quote(inst_id)
        );

        if (res.empty()) {
            apiLogger.debug("Nenhuma instância encontrada no banco principal.");
            return false;
        }

        bool current_is_active = res[0][0].as<bool>();

        if (current_is_active != is_active) {
            apiLogger.info("Atualizando status de atividade da instância: " + inst_id + " para " + (is_active ? "ativo" : "inativo"));
            wrk.exec("UPDATE instances SET is_active = " + std::string(is_active ? "true" : "false") +
                     " WHERE instance_id = " + wrk.quote(inst_id));
            wrk.commit();
        } else {
            wrk.abort();
        }

        return is_active;

    } catch (const std::exception& e) {
        apiLogger.error("Erro ao verificar/atualizar status da instância: " + std::string(e.what()));
        return is_active;
    }
}
