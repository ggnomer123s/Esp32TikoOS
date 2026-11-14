#pragma once
#include <stdbool.h>
#ifdef __cplusplus
extern "C" {
#endif
bool tiko_web_start(bool ap, const char *password);
void tiko_web_stop(void);
bool tiko_web_ip(char *out, unsigned size);
#ifdef __cplusplus
}
#endif
