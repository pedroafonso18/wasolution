#include "config.h"
#include "../../dependencies/dotenv.h"

Config::Config() {
    dotenv::init();
    loadEnv();
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