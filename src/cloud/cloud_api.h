#pragma once
#include "constants.h"
#include "cloud_constants.h"
#include <curl/curl.h>
#include "../api/api_constants.h"

class Cloud {
private:
    static Status subscribeToWaba_(std::string waba_id, std::string access_token);
    static Status getPhoneNumberId_(std::string waba_id, std::string access_token);
    static Status registerPhoneNumber_(std::string phone_number_id, std::string access_token);
public:
    static Status registerNumber(std::string waba_id, std::string access_token);
    static Status sendMessage(std::string instance_id, std::string receiver, std::string body, MediaType m_type, std::string phone_number_id, std::string access_token);
    static Status registerTemplate(std::string access_token, Template template_, std::string inst_id, std::string waba_id);
    static Status sendTemplate(std::string instance_id, std::string receiver, std::string body, MediaType m_type, std::string phone_number_id, std::string access_token, std::vector<FB_VARS> vars, std::string template_name);
};