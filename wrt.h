#ifndef WRT_H
#define WRT_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define MAX_URL_LENGTH 2048
#define DEFAULT_REQUESTS 10
#define DEFAULT_THREADS 5

typedef struct {
    char url[MAX_URL_LENGTH];
    int requests;
    int threads;
    int timeout;
    int pause;
    int insecure;
} wrt_config_t;

// Función principal para iniciar el ataque
void wrt_start(wrt_config_t *config);

// Inicializa una configuración por defecto
void wrt_init_config(wrt_config_t *config);

// Función que realiza una solicitud (usada en múltiples hilos)
void *wrt_perform_request(void *arg);

// Imprime ayuda de uso
void wrt_print_usage(const char *program_name);

#endif
