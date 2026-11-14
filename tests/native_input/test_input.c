#include <assert.h>
#include <stdio.h>
#include "target_input.h"
#include "input_wake.h"
#include "firmware/targets/lilygo_t_embed_cc1101/tiko_input.h"
static int pins[64];static uint32_t clock_ms,sequence;static InputEvent events[512];static unsigned count;static FuriPubSub bus;
uint32_t furi_get_tick(void){return clock_ms;}
uint32_t furi_ms_to_ticks(uint32_t ms){return ms;}
int gpio_get_level(gpio_num_t pin){assert(pin>=0&&pin<64);return pins[pin];}
int gpio_config(const gpio_config_t* c){(void)c;return 0;}
void furi_hal_reboot_combo_init(void){}
bool input_wake_init(const gpio_num_t* p,size_t n){(void)p;(void)n;return true;}
bool input_wake_wait(const InputWakePin* p,size_t n,uint32_t timeout){(void)p;(void)n;(void)timeout;return false;}
void furi_pubsub_publish(FuriPubSub*p,const void* e){(void)p;assert(count<512);events[count++]=*(const InputEvent*)e;}
static void poll(unsigned ms){for(unsigned i=0;i<ms;i+=4){clock_ms+=4;target_input_poll(&bus,&sequence);}}
static unsigned shorts(InputKey key){unsigned n=0;for(unsigned i=0;i<count;i++)if(events[i].key==key&&events[i].type==InputTypeShort)n++;return n;}
static void ab(int state){pins[4]=(state>>1)&1;pins[5]=state&1;poll(8);}
static void click(){pins[0]=0;poll(40);pins[0]=1;poll(16);}
int main(void){for(int i=0;i<64;i++)pins[i]=1;clock_ms=1000;target_input_init();poll(16);
 click();assert(shorts(InputKeyOk)==1);count=0;
 // A genuine short click directly after turning must not require a second press.
 ab(1);ab(0);unsigned rotations=shorts(InputKeyUp)+shorts(InputKeyDown);assert(rotations==1);click();assert(shorts(InputKeyOk)==1);count=0;
 // Shaft jostle exactly when the switch is pressed must not move the selection.
 pins[0]=0;ab(2);ab(3);pins[0]=1;poll(16);assert(shorts(InputKeyOk)==1);assert(shorts(InputKeyUp)+shorts(InputKeyDown)==0);count=0;
 tiko_input_configure(true,1);ab(1);ab(0);assert(shorts(InputKeyUp)==1);count=0;
 tiko_input_configure(false,2);ab(2);ab(3);assert(shorts(InputKeyDown)==0);ab(1);ab(0);assert(shorts(InputKeyDown)==1);count=0;
 ab(3);TikoInputStats stats=tiko_input_snapshot();assert(stats.invalid>0);assert(stats.transitions>0);
 pins[6]=0;poll(32);pins[6]=1;poll(16);assert(shorts(InputKeyBack)==1);
 puts("Native target_input: click, immediate post-scroll click, shaft jostle, reverse, sensitivity, invalid AB, Back passed");
}
