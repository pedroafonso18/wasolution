#pragma once

#include <curl/curl.h>
#include "../constants.h"
#include <string>
#include <format>
#include <iostream>

using std::string;

class Evolution {
    public:
        typedef struct {
            string host;
            string port;
            string protocol;
            string username;
            string password;
        } Proxy;

        Evolution() = delete;

        static Status sendMessage_e(string phone, string token, string url, MediaType type, string msg_template, string instance_name);
        static Status createInstance_e(string evo_token, string inst_token,string inst_name, string url, string webhook_url, Proxy proxy_url);
        static Status deleteInstance_e(string inst_token, string evo_token, string url);
};