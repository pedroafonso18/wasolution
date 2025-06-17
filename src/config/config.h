#pragma once

#include <string>
#include "../constants.h"

typedef struct {
    std::string evo_url;
    std::string evo_token;
    std::string wuz_url;
    std::string db_url;
    std::string wuz_token;
    std::string callback_url;
} Env;

class Config{
    private:
        Env env_vars;
        void loadEnv();
        std::string getLocalIP();
    public:
        Config();
        const Env& getEnv() const;
};