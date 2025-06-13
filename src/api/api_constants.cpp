#include "api_constants.h"

#ifdef __cplusplus
extern "C" {
#endif

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t totalSize = size * nmemb;
    auto* response = static_cast<std::string*>(userp);
    response->append(static_cast<char*>(contents), totalSize);
    return totalSize;
}

#ifdef __cplusplus
}
#endif
