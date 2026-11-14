#pragma once
#include <stdint.h>
typedef int gpio_num_t;
typedef int gpio_int_type_t;
typedef struct{uint64_t pin_bit_mask;int mode,pull_up_en,pull_down_en,intr_type;} gpio_config_t;
enum{GPIO_MODE_INPUT,GPIO_PULLUP_ENABLE,GPIO_PULLUP_DISABLE,GPIO_PULLDOWN_ENABLE,GPIO_PULLDOWN_DISABLE,GPIO_INTR_DISABLE,GPIO_INTR_LOW_LEVEL,GPIO_INTR_HIGH_LEVEL};
int gpio_get_level(gpio_num_t);
int gpio_config(const gpio_config_t*);
