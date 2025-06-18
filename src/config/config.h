#pragma once

#include <string>
#include "../constants.h"

typedef struct {
    std::string evo_url;
    std::string evo_token;
    std::string wuz_url;
    std::string db_url;
    std::string db_url_wuz;
    std::string default_webhook;
    std::string ip_address;
} Env;

class Config{
    private:
        Env env_vars;
        void loadEnv();
    public:
        Config();
        const Env& getEnv() const;
        const std::string getIpAddress() const;
};