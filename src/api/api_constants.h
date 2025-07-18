#pragma once

#include <string>
#include <curl/curl.h>

#ifdef __cplusplus
extern "C" {
#endif

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
bool isHttpResponseOk(CURL* curl);

#ifdef __cplusplus
}
#endif

static std::string getMimeTypeExtensions(const std::string& mime_type);