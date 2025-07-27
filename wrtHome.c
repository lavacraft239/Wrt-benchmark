#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <ctype.h>

#define CSV_FILE "wrt_results.csv"
#define MAX_LINE 1024
#define HISTORIAL 3

// Colores ANSI
#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define CYAN  "\033[0;36m"
#define RESET "\033[0m"

int check_server(const char *url) {
    CURL *curl = curl_easy_init();
    if (!curl) return 0;

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);

    CURLcode res = curl_easy_perform(curl);
    long response_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
    curl_easy_cleanup(curl);

    return (res == CURLE_OK && response_code >= 200 && response_code < 400);
}

void limpiar_url(char *url) {
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        char nueva[512];
        snprintf(nueva, sizeof(nueva), "http://%s", url);
        strncpy(url, nueva, 256);
    }
}

void mostrar_linea(char *linea) {
    char *token;
    char url[256] = "", timestamp[64] = "";
    int campo = 0, threads = 0, requests = 0, timeouts = 0;
    float avg = 0;

    token = strtok(linea, ",");
    while (token) {
        switch (campo) {
            case 0: strncpy(timestamp, token, sizeof(timestamp)); break;
            case 1: strncpy(url, token, sizeof(url)); break;
            case 2: threads = atoi(token); break;
            case 5: requests = atoi(token); break;
            case 6: /* errors, no usado */ break;
            case 7: timeouts = atoi(token); break;
            case 8: avg = atof(token); break;
        }
        token = strtok(NULL, ",");
        campo++;
    }

    limpiar_url(url);
    printf(CYAN "▶ [%s]\n" RESET, timestamp);
    printf("URL: %s\n", url);
    printf("Hilos: %d | Requests: %d | Timeouts: %d | Promedio: %.2f ms\n", threads, requests, timeouts, avg);

    int vivo = check_server(url);
    printf("Estado del servidor: %s%s%s\n\n", vivo ? GREEN : RED, vivo ? "🟢 Vivo" : "🔴 Caído", RESET);
}

void mostrar_historial() {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) {
        printf(RED "No se encontró el archivo '%s'.\n" RESET, CSV_FILE);
        return;
    }

    // Contar líneas
    char **lineas = malloc(sizeof(char*) * 5000);
    int total = 0;
    char buffer[MAX_LINE];
    while (fgets(buffer, sizeof(buffer), f)) {
        lineas[total] = strdup(buffer);
        total++;
    }
    fclose(f);

    if (total == 0) {
        printf("El archivo está vacío.\n");
        return;
    }

    printf("Mostrando últimos %d ataques:\n\n", HISTORIAL);
    int desde = total - HISTORIAL;
    if (desde < 0) desde = 0;
    for (int i = desde; i < total; i++) {
        mostrar_linea(lineas[i]);
        free(lineas[i]);
    }
    free(lineas);
}

int main() {
    printf(CYAN "=== Monitor de WRT ===\n\n" RESET);
    mostrar_historial();
    return 0;
}
