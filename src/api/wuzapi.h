#pragma once

#include <curl/curl.h>
#include <iostream>
#include <string>
#include <format>
#include "../constants.h"
#include "api_constants.h"

using std::string;

class Wuzapi {
private:

public:
    Wuzapi() = delete;
    static Status sendMessage_w(string phone, string token, string url, MediaType type, string msg_template);
    static Status createInstance_w(string inst_token, string url, string webhook_url, string proxy_url);
    static Status deleteInstance_w(string inst_token, string url);
    static Status connectInstance_w(string inst_token, string url);
    static Status logoutInstance_w(string inst_token, string url);
    static Status setWebhook_w(string token, string webhook_url, string url);
    static Status setProxy_w(string token, string proxy_url, string url);
    static Status getQrCode_w(string token, string url);
};