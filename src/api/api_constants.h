#pragma once

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);

#ifdef __cplusplus
}
#endif
