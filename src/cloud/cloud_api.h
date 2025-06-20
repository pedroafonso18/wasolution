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
    static Status sendMessage(std::string instance_id, )
};