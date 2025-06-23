#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define MAX_URL_LEN 2048
#define CSV_FILE "wrt_results.csv"

typedef struct {
    char url[MAX_URL_LEN];
    int threads;
    int connections;
    int duration; // segundos
    int timeout;  // segundos
    int insecure; // ignorar certificado SSL
} config_t;

typedef struct {
    int requests;
    int errors;
    int timeouts;
    double total_latency;
    double min_latency;
    double max_latency;
    pthread_mutex_t lock;
} stats_t;

stats_t stats = {0, 0, 0, 0.0, 9999.0, 0.0, PTHREAD_MUTEX_INITIALIZER};

typedef struct {
    config_t *config;
    time_t start_time;
} thread_arg_t;

double now_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata) {
    return size * nmemb; // ignorar cuerpo
}

void print_progress_bar(int elapsed, int total) {
    int width = 30;
    int filled = (elapsed * width) / total;
    printf("\r[");
    for (int i = 0; i < width; ++i) {
        printf(i < filled ? "#" : ".");
    }
    printf("] %d%% (%ds/%ds)", (elapsed * 100) / total, elapsed, total);
    fflush(stdout);
}

void *progress_bar_thread(void *arg) {
    config_t *cfg = (config_t *)arg;
    time_t start = time(NULL);
    while (1) {
        int elapsed = time(NULL) - start;
        if (elapsed > cfg->duration) break;
        print_progress_bar(elapsed, cfg->duration);
        sleep(1);
    }
    print_progress_bar(cfg->duration, cfg->duration);
    printf("\n");
    return NULL;
}

void *worker(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    config_t *cfg = targ->config;

    while (time(NULL) - targ->start_time < cfg->duration) {
        CURL *curl = curl_easy_init();
        if (!curl) continue;

        curl_easy_setopt(curl, CURLOPT_URL, cfg->url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, cfg->timeout);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        if (cfg->insecure) {
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: wrt/1.1");
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

        double start = now_ms();
        CURLcode res = curl_easy_perform(curl);
        double latency = (now_ms() - start) / 1000.0;

        long code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);

        pthread_mutex_lock(&stats.lock);
        if (res == CURLE_OK && code >= 200 && code < 400) {
            stats.requests++;
            stats.total_latency += latency;
            if (latency < stats.min_latency) stats.min_latency = latency;
            if (latency > stats.max_latency) stats.max_latency = latency;
        } else if (res == CURLE_OPERATION_TIMEDOUT) {
            stats.timeouts++;
        } else {
            stats.errors++;
        }
        pthread_mutex_unlock(&stats.lock);

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return NULL;
}

void print_usage(char *prog) {
    printf("Uso: %s URL -r threads -p conexiones -t duracion -w timeout [--insecure]\n", prog);
}

int main(int argc, char *argv[]) {
    if (argc < 9) {
        print_usage(argv[0]);
        return 1;
    }

    config_t cfg = {0};
    strncpy(cfg.url, argv[1], MAX_URL_LEN - 1);

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            cfg.threads = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            cfg.connections = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            cfg.duration = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            cfg.timeout = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--insecure")) {
            cfg.insecure = 1;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    printf("Lanzando prueba: %s\n", cfg.url);
    printf("Threads: %d, Conexiones: %d, Duración: %ds, Timeout: %ds\n",
           cfg.threads, cfg.connections, cfg.duration, cfg.timeout);
    if (cfg.insecure) {
        printf("⚠️ Ignorando verificación SSL (modo --insecure)\n");
    }
    printf("\n");

    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t *tids = malloc(sizeof(pthread_t) * cfg.threads);
    pthread_t progress_tid;

    thread_arg_t targ = {.config = &cfg, .start_time = time(NULL)};

    pthread_create(&progress_tid, NULL, progress_bar_thread, &cfg);

    for (int i = 0; i < cfg.threads; i++) {
        pthread_create(&tids[i], NULL, worker, &targ);
    }

    for (int i = 0; i < cfg.threads; i++) {
        pthread_join(tids[i], NULL);
    }

    pthread_join(progress_tid, NULL);

    printf("\n--- Resultados ---\n");
    printf("Requests: %d\n", stats.requests);
    printf("Errors: %d\n", stats.errors);
    printf("Timeouts: %d\n", stats.timeouts);
    if (stats.requests > 0) {
        printf("Latencia promedio: %.3f seg\n", stats.total_latency / stats.requests);
        printf("Latencia min: %.3f seg | max: %.3f seg\n", stats.min_latency, stats.max_latency);
    }

    FILE *f = fopen(CSV_FILE, "a");
    if (f) {
        time_t now = time(NULL);
        fprintf(f, "%ld,%s,%d,%d,%d,%d,%d,%d,%.3f,%.3f,%.3f\n",
                now, cfg.url, cfg.threads, cfg.connections, cfg.duration,
                stats.requests, stats.errors, stats.timeouts,
                stats.requests ? stats.total_latency / stats.requests : 0,
                stats.min_latency, stats.max_latency);
        fclose(f);
    }

    free(tids);
    curl_global_cleanup();
    return 0;
}
