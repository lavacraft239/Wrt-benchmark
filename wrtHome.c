#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#define CSV_FILE "wrt_results.csv"
#define MAX_LINE 1024

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

void mostrar_ultimo_resultado() {
    FILE *f = fopen(CSV_FILE, "r");
    if (!f) {
        printf("No se encontró el archivo de resultados '%s'.\n", CSV_FILE);
        return;
    }

    char linea[MAX_LINE];
    char ultima[MAX_LINE];
    while (fgets(linea, sizeof(linea), f)) {
        strcpy(ultima, linea);
    }
    fclose(f);

    // Formato esperado: timestamp,url,threads,conn,duration,requests,errors,timeouts,avg,min,max
    char *token = strtok(ultima, ",");
    int campo = 0;
    char url[256];
    int threads = 0, requests = 0, timeouts = 0;

    while (token) {
        switch (campo) {
            case 1: strncpy(url, token, sizeof(url)); break;
            case 2: threads = atoi(token); break;
            case 5: requests = atoi(token); break;
            case 7: timeouts = atoi(token); break;
        }
        token = strtok(NULL, ",");
        campo++;
    }

    printf("Último ataque a: %s\n", url);
    printf("Hilos: %d\n", threads);
    printf("Requests: %d\n", requests);
    printf("Timeouts: %d\n", timeouts);

    int vivo = check_server(url);
    printf("Estado del servidor: %s\n", vivo ? "🟢 Vivo" : "🔴 Caído");
}

int main() {
    printf("=== Monitor de WRT ===\n\n");
    mostrar_ultimo_resultado();
    return 0;
}
