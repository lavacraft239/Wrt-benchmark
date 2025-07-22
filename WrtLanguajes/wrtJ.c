// wrt_lib.c
#include "wrt.h"

void wrt_start_from_java(const char *url) {
    wrt_config_t config;
    wrt_init_config(&config);
    strncpy(config.url, url, sizeof(config.url));
    wrt_start(&config);
}
