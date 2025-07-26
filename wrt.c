#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h> // Para WEXITSTATUS

#define MAX_URL_LEN 2048
#define CSV_FILE "wrt_results.csv"

typedef struct {
    char url[MAX_URL_LEN];
    int threads;
    int connections;
    int duration;        // segundos
    int timeout_total;   // CURLOPT_TIMEOUT
    int timeout_connect; // CURLOPT_CONNECTTIMEOUT
    int insecure;        // ignorar certificado SSL
    int ignore_url;      // simular petición (sin hacer request real)
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

typedef struct {
    CURL *easy_handle;
    double start_time_ms;
    config_t *config;
    int active;
} easy_request_t;

void check_and_update_stats(CURL *curl, CURLcode res, double latency) {
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
}

void *worker_multi(void *arg) {
    thread_arg_t *targ = (thread_arg_t *)arg;
    config_t *cfg = targ->config;

    if (cfg->ignore_url) {
        while (time(NULL) - targ->start_time < cfg->duration) {
            usleep((cfg->timeout_total > 0 ? cfg->timeout_total : 5) * 100000);
            pthread_mutex_lock(&stats.lock);
            stats.requests++;
            pthread_mutex_unlock(&stats.lock);
        }
        return NULL;
    }

    CURLM *multi_handle = curl_multi_init();
    if (!multi_handle) return NULL;

    int max_handles = cfg->connections > 0 ? cfg->connections : 10;
    easy_request_t *requests = calloc(max_handles, sizeof(easy_request_t));

    for (int i = 0; i < max_handles; i++) {
        CURL *easy = curl_easy_init();
        requests[i].easy_handle = easy;
        requests[i].config = cfg;
        requests[i].active = 0;

        curl_easy_setopt(easy, CURLOPT_URL, cfg->url);
        curl_easy_setopt(easy, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(easy, CURLOPT_TIMEOUT, cfg->timeout_total);
        curl_easy_setopt(easy, CURLOPT_CONNECTTIMEOUT, cfg->timeout_connect);
        curl_easy_setopt(easy, CURLOPT_FOLLOWLOCATION, 1L);

        if (cfg->insecure) {
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYPEER, 0L);
            curl_easy_setopt(easy, CURLOPT_SSL_VERIFYHOST, 0L);
        }

        struct curl_slist *headers = NULL;
        headers = curl_slist_append(headers, "User-Agent: wrt/1.2");
        curl_easy_setopt(easy, CURLOPT_HTTPHEADER, headers);
    }

    int added = 0;
    for (; added < max_handles && time(NULL) - targ->start_time < cfg->duration; added++) {
        curl_multi_add_handle(multi_handle, requests[added].easy_handle);
        requests[added].start_time_ms = now_ms();
        requests[added].active = 1;
    }

    int still_running = 0;
    do {
        CURLMcode mc = curl_multi_perform(multi_handle, &still_running);
        if (mc != CURLM_OK) break;

        int msgs_in_queue = 0;
        CURLMsg *msg;
        while ((msg = curl_multi_info_read(multi_handle, &msgs_in_queue))) {
            if (msg->msg == CURLMSG_DONE) {
                CURL *e = msg->easy_handle;
                double latency = 0.0;
                for (int i = 0; i < max_handles; i++) {
                    if (requests[i].easy_handle == e) {
                        latency = (now_ms() - requests[i].start_time_ms) / 1000.0;
                        break;
                    }
                }

                check_and_update_stats(e, msg->data.result, latency);

                curl_multi_remove_handle(multi_handle, e);
                curl_easy_reset(e);

                curl_easy_setopt(e, CURLOPT_URL, cfg->url);
                curl_easy_setopt(e, CURLOPT_WRITEFUNCTION, write_callback);
                curl_easy_setopt(e, CURLOPT_TIMEOUT, cfg->timeout_total);
                curl_easy_setopt(e, CURLOPT_CONNECTTIMEOUT, cfg->timeout_connect);
                curl_easy_setopt(e, CURLOPT_FOLLOWLOCATION, 1L);
                if (cfg->insecure) {
                    curl_easy_setopt(e, CURLOPT_SSL_VERIFYPEER, 0L);
                    curl_easy_setopt(e, CURLOPT_SSL_VERIFYHOST, 0L);
                }
                struct curl_slist *headers = NULL;
                headers = curl_slist_append(headers, "User-Agent: wrt/1.2");
                curl_easy_setopt(e, CURLOPT_HTTPHEADER, headers);

                if (time(NULL) - targ->start_time < cfg->duration) {
                    curl_multi_add_handle(multi_handle, e);
                    for (int i = 0; i < max_handles; i++) {
                        if (requests[i].easy_handle == e) {
                            requests[i].start_time_ms = now_ms();
                            requests[i].active = 1;
                            break;
                        }
                    }
                } else {
                    for (int i = 0; i < max_handles; i++) {
                        if (requests[i].easy_handle == e) {
                            requests[i].active = 0;
                            break;
                        }
                    }
                }
            }
        }

        curl_multi_wait(multi_handle, NULL, 0, 1000, NULL);

    } while (still_running > 0);

    for (int i = 0; i < max_handles; i++) {
        if (requests[i].easy_handle) {
            curl_easy_cleanup(requests[i].easy_handle);
        }
    }
    free(requests);
    curl_multi_cleanup(multi_handle);

    return NULL;
}

void print_usage(char *prog) {
    printf("Uso: %s URL -r threads -p conexiones -t duracion -w timeout_total -c timeout_connect [--insecure] [-x] [--small]\n", prog);
    printf("  --insecure  Ignorar verificación SSL\n");
    printf("  -x          Ignorar URL (simular carga sin hacer peticiones)\n");
    printf("  --small     Modo rápido con parámetros por defecto (r=2, p=10, t=10, w=5, c=2)\n");
    printf("\nEjemplo:\n");
    printf("  %s https://ejemplo.com -r 10 -p 100 -t 30 -w 10 -c 5 --insecure\n", prog);
    printf("  %s rtmp://ejemplo.com/stream -r 5 -p 50 -t 60 -w 20 -c 10\n", prog);
    printf("  %s http://localhost --small\n", prog);
}

// Función para test RTMP usando ffmpeg
int test_rtmp(const char *url, int duration) {
    char cmd[4096];
    snprintf(cmd, sizeof(cmd),
        "ffmpeg -loglevel error -rw_timeout 20000000 -i \"%s\" -t %d -f null - 2>&1",
        url, duration);

    printf("Ejecutando comando RTMP con ffmpeg:\n%s\n", cmd);

    FILE *pipe = popen(cmd, "r");
    if (!pipe) {
        fprintf(stderr, "Error: no se pudo ejecutar ffmpeg\n");
        return -1;
    }

    char buffer[1024];
    while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
        // Podés analizar la salida si querés
        // printf("%s", buffer);
    }

    int status = pclose(pipe);
    if (WIFEXITED(status)) {
        int exit_status = WEXITSTATUS(status);
        if (exit_status == 0) {
            printf("RTMP stream accesible y probado correctamente.\n");
            return 0;
        } else {
            printf("ffmpeg finalizó con error, código: %d\n", exit_status);
            return exit_status;
        }
    } else {
        printf("ffmpeg terminó inesperadamente.\n");
        return -1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    config_t cfg = {0};
    strncpy(cfg.url, argv[1], MAX_URL_LEN - 1);
    cfg.timeout_total = 10;
    cfg.timeout_connect = 5;

    int small_mode = 0;

    for (int i = 2; i < argc; i++) {
        if (!strcmp(argv[i], "--small")) {
            small_mode = 1;
        } else if (!strcmp(argv[i], "-r") && i + 1 < argc) {
            cfg.threads = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-p") && i + 1 < argc) {
            cfg.connections = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-t") && i + 1 < argc) {
            cfg.duration = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-w") && i + 1 < argc) {
            cfg.timeout_total = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "-c") && i + 1 < argc) {
            cfg.timeout_connect = atoi(argv[++i]);
        } else if (!strcmp(argv[i], "--insecure")) {
            cfg.insecure = 1;
        } else if (!strcmp(argv[i], "-x")) {
            cfg.ignore_url = 1;
        } else {
            print_usage(argv[0]);
            return 1;
        }
    }

    if (small_mode) {
        if (cfg.threads == 0) cfg.threads = 2;
        if (cfg.connections == 0) cfg.connections = 10;
        if (cfg.duration == 0) cfg.duration = 10;
        if (cfg.timeout_total == 10) cfg.timeout_total = 5;       // solo si no cambió
        if (cfg.timeout_connect == 5) cfg.timeout_connect = 2;    // solo si no cambió

        printf("🔹 Modo --small activado: r=%d, p=%d, t=%d, w=%d, c=%d\n",
               cfg.threads, cfg.connections, cfg.duration, cfg.timeout_total, cfg.timeout_connect);
    }

    if (cfg.threads == 0 || cfg.connections == 0 || cfg.duration == 0) {
        fprintf(stderr, "❌ Error: faltan parámetros obligatorios (-r -p -t). Usa --small para un modo automático.\n");
        print_usage(argv[0]);
        return 1;
    }

    if (strncmp(cfg.url, "rtmp://", 7) == 0) {
        printf("(Anuncio)Hay una nueva desarrolladora que es Camila \n");
        printf("Lanzando prueba RTMP real con ffmpeg: %s\n", cfg.url);

        int ret = test_rtmp(cfg.url, cfg.duration);

        printf("\n--- Resultados ---\n");
        if (ret == 0) {
            printf("RTMP Requests: 1 (simulado)\n");
            printf("RTMP Errors: 0\n");
            printf("RTMP Timeouts: 0\n");
        } else {
            printf("RTMP Requests: 0\n");
            printf("RTMP Errors: 1\n");
            printf("RTMP Timeouts: 0\n");
        }
        return ret == 0 ? 0 : 1;
    }

    printf("(Anuncio)Hay una nueva desarrolladora que es Camila \n");
    printf("Lanzando prueba: %s\n", cfg.url);
    printf("Threads: %d, Conexiones: %d, Duración: %ds, Timeout total: %ds, Timeout conexión: %ds\n",
           cfg.threads, cfg.connections, cfg.duration, cfg.timeout_total, cfg.timeout_connect);
    if (cfg.insecure) {
        printf("⚠️ Ignorando verificación SSL (modo --insecure)\n");
    }
    if (cfg.ignore_url) {
        printf("⚠️ Modo ignorar URL activado (simulación sin peticiones)\n");
    }
    printf("\n");

    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t *tids = malloc(sizeof(pthread_t) * cfg.threads);
    pthread_t progress_tid;

    thread_arg_t targ = {.config = &cfg, .start_time = time(NULL)};

    pthread_create(&progress_tid, NULL, progress_bar_thread, &cfg);

    for (int i = 0; i < cfg.threads; i++) {
        pthread_create(&tids[i], NULL, worker_multi, &targ);
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
