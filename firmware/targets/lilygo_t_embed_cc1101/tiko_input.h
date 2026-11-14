#pragma once
#include <stdbool.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct {
    uint32_t transitions, invalid;
    int32_t steps;
    uint8_t ab;
} TikoInputStats;
void tiko_input_configure(bool reverse, unsigned sensitivity);
TikoInputStats tiko_input_snapshot(void);
#ifdef __cplusplus
}
#endif
