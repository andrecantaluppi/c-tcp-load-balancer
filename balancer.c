#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PROXY_PORT 8080
#define BUFFER_SIZE 4096
#define NUM_BACKENDS 3
#define HEALTH_CHECK_INTERVAL 3 // Secondi tra un check e l'altro

typedef struct {
    char *ip;
    int port;
    int is_alive; // 1 se online, 0 se offline
} Backend;

Backend backends[NUM_BACKENDS] = {
    {"127.0.0.1", 8001, 0},
    {"127.0.0.1", 8002, 0},
    {"127.0.0.1", 8003, 0}
};

int current_backend = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

// Thread in background per gli Active Health Checks
void *health_check_loop(void *arg) {
    while (1) {
        for (int i = 0; i < NUM_BACKENDS; i++) {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            struct sockaddr_in addr;
            addr.sin_family = AF_INET;
            addr.sin_port = htons(backends[i].port);
            inet_pton(AF_INET, backends[i].ip, &addr.sin_addr);

            // Prova a connettersi senza inviare dati
            int is_connected = (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) == 0);
            close(sock);

            pthread_mutex_lock(&lock);
            // Stampa un avviso solo se lo stato cambia
            if (backends[i].is_alive != is_connected) {
                backends[i].is_alive = is_connected;
                printf("[Health Check] Backend %s:%d è ora %s\n", 
                       backends[i].ip, backends[i].port, is_connected ? "ONLINE" : "OFFLINE");
            }
            pthread_mutex_unlock(&lock);
        }
        sleep(HEALTH_CHECK_INTERVAL);
    }
    return NULL;
}

// Trova il prossimo server ONLINE usando Round-Robin
int get_next_alive_backend_index() {
    pthread_mutex_lock(&lock);
    int start_index = current_backend;
    int found_index = -1;

    do {
        if (backends[current_backend].is_alive) {
            found_index = current_backend;
            current_backend = (current_backend + 1) % NUM_BACKENDS;
            break;
        }
        current_backend = (current_backend + 1) % NUM_BACKENDS;
    } while (current_backend != start_index);

    pthread_mutex_unlock(&lock);
    return found_index;
}

void *handle_connection(void *arg) {
    int client_fd = *(int *)arg;
    free(arg);

    int backend_idx = get_next_alive_backend_index();
    
    if (backend_idx == -1) {
        printf("[Proxy] Nessun backend disponibile!\n");
        char *error_msg = "503 Service Unavailable\n";
        write(client_fd, error_msg, strlen(error_msg));
        close(client_fd);
        return NULL;
    }

    Backend backend = backends[backend_idx];
    printf("[Proxy] Inoltro traffico al backend %s:%d\n", backend.ip, backend.port);

    int backend_fd = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in backend_addr;
    backend_addr.sin_family = AF_INET;
    backend_addr.sin_port = htons(backend.port);
    inet_pton(AF_INET, backend.ip, &backend_addr.sin_addr);

    if (connect(backend_fd, (struct sockaddr *)&backend_addr, sizeof(backend_addr)) < 0) {
        close(backend_fd);
        close(client_fd);
        return NULL;
    }

    char buffer[BUFFER_SIZE];
    int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
    if (bytes_read > 0) {
        write(backend_fd, buffer, bytes_read);
        
        // Loop reading from backend until EOF
        while (1) {
            int bytes_resp = read(backend_fd, buffer, BUFFER_SIZE - 1);
            if (bytes_resp <= 0) break;
            write(client_fd, buffer, bytes_resp);
        }
    }

    close(backend_fd);
    close(client_fd);
    return NULL;
}

#include <signal.h>

int main() {
    // Ignora SIGPIPE per evitare crash se il client si disconnette
    signal(SIGPIPE, SIG_IGN);

    // Avvia il thread di Health Check in background
    pthread_t health_thread;
    pthread_create(&health_thread, NULL, health_check_loop, NULL);
    pthread_detach(health_thread);

    int server_fd, *new_sock;
    struct sockaddr_in address;
    int opt = 1;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PROXY_PORT);

    bind(server_fd, (struct sockaddr *)&address, sizeof(address));
    listen(server_fd, 10);
    
    printf("Load Balancer con Health Checks attivo sulla porta %d...\n", PROXY_PORT);

    while (1) {
        int client_fd = accept(server_fd, NULL, NULL);
        new_sock = malloc(sizeof(int));
        *new_sock = client_fd;

        pthread_t thread_id;
        pthread_create(&thread_id, NULL, handle_connection, (void *)new_sock);
        pthread_detach(thread_id);
    }
    
    return 0;
}