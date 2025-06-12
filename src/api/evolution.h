#pragma once

#include <curl/curl.h>
#include "../constants.h"
#include <string>
#include <format>
#include <iostream>
#include "api_constants.h"

using std::string;

class Evolution {
    public:
        typedef struct {
            std::string host;
            std::string port;
            std::string protocol;
            std::string username;
            std::string password;
        } Proxy;

        Evolution() = delete;

        static Status sendMessage_e(string phone, string token, string url, MediaType type, string msg_template, string instance_name);
        static Status createInstance_e(string evo_token, string inst_token,string inst_name, string url, string webhook_url, std::string proxy_url);
        static Status deleteInstance_e(string inst_token, string evo_token, string url);
        static Proxy ParseProxy(std::string proxy_url);
};