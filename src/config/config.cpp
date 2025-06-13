#include "config.h"
#include "../../dependencies/dotenv.h"
#include <filesystem>
#include <iostream>

Config::Config() {
    std::string root_path = "../.env";
    if (std::filesystem::exists(".env")) {
        root_path = ".env";
    }

    std::cout << "Tentando carregar .env de: " << root_path << std::endl;
    dotenv::init(root_path.c_str());
    loadEnv();

    std::cout << "EVO_URL carregada: [" << env_vars.evo_url << "]" << std::endl;
}


void Config::loadEnv() {
    env_vars.evo_url = dotenv::getenv("EVO_URL", "");
    env_vars.evo_token = dotenv::getenv("EVO_TOKEN", "");
    env_vars.wuz_url = dotenv::getenv("WUZ_URL", "");
    env_vars.db_url = dotenv::getenv("DB_URL", "");
    env_vars.wuz_token = dotenv::getenv("WUZ_TOKEN", "");
}

const Env& Config::getEnv() const {
    return env_vars;
}