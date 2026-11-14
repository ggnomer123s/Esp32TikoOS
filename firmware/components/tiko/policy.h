#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
bool tiko_url_decode(const char *raw, char *out, size_t size);
bool tiko_policy_path(const char *path);
bool tiko_policy_upload(const char *path, uint64_t bytes);
#ifdef __cplusplus
}
#endif
