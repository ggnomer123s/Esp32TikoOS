#pragma once
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "input.h"
typedef struct{int unused;} FuriPubSub;
uint32_t furi_get_tick(void);
uint32_t furi_ms_to_ticks(uint32_t ms);
void furi_pubsub_publish(FuriPubSub*,const void*);
void target_input_init(void);
void target_input_poll(FuriPubSub*,uint32_t*);
#define FURI_LOG_I(...) ((void)0)
#define FURI_LOG_E(...) ((void)0)
#define COUNT_OF(x) (sizeof(x)/sizeof((x)[0]))
