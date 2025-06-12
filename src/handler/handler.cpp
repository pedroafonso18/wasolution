#include "handler.h"

using std::string;

Status Handler::sendMessage(string instance_id, string number, string body, MediaType type, ApiType api) {
    Config config;
    auto env = config.getEnv();
    if (api == ApiType::EVOLUTION) {
        Evolution::sendMessage_e(number, env.evo_token, env.evo_url, type, body, )
    }
}