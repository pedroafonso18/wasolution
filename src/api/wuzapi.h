#pragma once;

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <format>
#include "../constants.h"

using std::string;

class Wuzapi {
    public:
        Wuzapi() = delete;
        Status sendMessage_w(string phone, string token, string url, MediaType type, string msg_template);
        Status createInstance_w(string wuz_token, string inst_token, string inst_name, string url, string webhook_url, string proxy_url);
        Status deleteInstance_w(string inst_token, string wuz_token, string url);
};