#pragma once

#include <string>

typedef struct {
    std::string evo_url;
    std::string evo_token;
    std::string wuz_url;
    std::string db_url;
    std::string wuz_token;
} Env;

class Config{
    private:
        Env env_vars;
        void loadEnv();
    public:
        Config();
        const Env& getEnv() const;
};