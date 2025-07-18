#include "api_constants.h"
#include <iostream>

#ifdef __cplusplus
extern "C" {
#endif

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

bool isHttpResponseOk(CURL* curl) {
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    std::cout << "HTTP Status Code: " << http_code << std::endl;
    return http_code >= 200 && http_code < 300;
}

#ifdef __cplusplus
}
#endif

std::string getMimeTypeExtensions(std::string mime_type) {
    if (mime_type == "png") return ".png";
    if (mime_type == "jpeg") return ".jpg";
    if (mime_type == "gif") return ".gif";
    if (mime_type == "pdf") return ".pdf";
    if (mime_type == "msword") return ".doc";
    if (mime_type == "vnd.openxmlformats-officedocument.wordprocessingml.document") return ".docx";
    if (mime_type == "plain") return ".txt";
    if (mime_type == "html") return ".html";
    if (mime_type == "mpeg") return ".mp3";
    if (mime_type == "wav") return ".wav";
    if (mime_type == "webm") return ".webm";
    if (mime_type == "mp4") return ".mp4";
    return "unknown";
}
