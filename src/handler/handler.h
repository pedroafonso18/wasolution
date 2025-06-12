#pragma once

#include <string>
#include "../constants.h"
#include "../api/evolution.h"
#include "../api/wuzapi.h"
#include "../config/config.h"

using std::string;


class Handler {
    public:
        Handler() = delete;

        static Status sendMessage(string instance_id, string number, string body, MediaType type, ApiType inst_type);
        static Status createInstance(string instance_id, ApiType api_type);
        static Status deleteInstance(string instance_id);
        static Status connectInstance(string instance_id);      
};