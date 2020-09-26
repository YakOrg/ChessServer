#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include "ll.h"

char *rand_string(char *str, size_t size) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    if (size)
        for (size_t n = 0; n < size; n++) {
            int key = rand() % (int) (sizeof charset - 1);
            str[n] = charset[key];
        }
    return str;
}

typedef struct Room {
    int *clients;
    char *invite_code;
} Room;

typedef struct Position {
    char xO;
    char yO;
    char xN;
    char yN;
} Position;


int send_data(int sock_fd, void *data, int len) {
    unsigned char *data_ptr = (unsigned char *) data;
    int numSent;

    while (len > 0) {
        numSent = send(sock_fd, data_ptr, len, 0);
        if (numSent == -1)
            return -1;
        data_ptr += numSent;
        len -= numSent;
    }

    return 0;
}

Room *create_room(ll_t *list, int client1_fd) {
    Room *new_room = malloc(sizeof(struct Room));
    new_room->clients = malloc(sizeof(int) * 2);
    new_room->clients[0] = client1_fd;
    new_room->clients[1] = -1;

    char *code = malloc(sizeof(char) * 7);
    code = rand_string(code, 7);
    code[3] = '-';
    new_room->invite_code = code;

    char test[8];
    strncpy(test, code, 7);
    test[7] = 0;
    printf("%s\n", test);

    ll_insert_last(list, new_room);
    return new_room;
}

int search_by_invite_code(void *data, void *args) {
    Room *room = data;
    char *code = args;
    return !strncmp(room->invite_code, code, 7);
}


int search_by_client_fd(void *data, void *args) {
    Room *room = data;
    int fd = *((int *) args);
    return room->clients[0] == fd || room->clients[1] == fd;
}

int get_other_client_fd(ll_t *list, int client_fd) {
    Room *room = ll_get_search(list, search_by_client_fd, &client_fd);
    if (room != NULL)
        return room->clients[0] == client_fd ? room->clients[1] : room->clients[0];

    return -1;
}

void join_room(ll_t *list, int client_fd, char *code) {
    Room *room = ll_get_search(list, search_by_invite_code, code);
    if (room != NULL)
        for (int i = 0; i < 2; ++i) {
            if (room->clients[i] == -1) {
                room->clients[i] = client_fd;
                printf("Client %d joined to room with %d\n", client_fd, get_other_client_fd(list, client_fd));
                return;
            }
        }
}

void free_room(void *memory) {
    Room *room = memory;
    free(room->invite_code);
    free(room);
}

typedef struct Process_args {
    ll_t *list;
    int sock_fd;
} Process_args;

void *process_client(void *arg) {
    Process_args *process_args = arg;
    int client_fd = process_args->sock_fd;
    ll_t *list = process_args->list;
    free(process_args);

    pthread_detach(pthread_self());

    char package_type;
    for (;;) {
        if (recv(client_fd, &package_type, 1, MSG_WAITALL) < 1)
            break;
        if (package_type == 0) {
            Room *room = ll_get_search(list, search_by_client_fd, &client_fd);
            if (room == 0) {
                room = create_room(list, client_fd);
                printf("Room created by %d\n", client_fd);
            }
            if (send_data(client_fd, room->invite_code, 7) == -1)
                break;
        } else if (package_type == 1) {
            char code[7];
            if (recv(client_fd, &code, 7, MSG_WAITALL) < 1)
                break;
            join_room(list, client_fd, code);
        } else if (package_type == 2) {
            Position position;
            if (recv(client_fd, &position, sizeof(position), MSG_WAITALL) < 1)
                break;
            int second_fd = get_other_client_fd(list, client_fd);
            if (send_data(second_fd, &position, sizeof(position)) == -1)
                break;
        } else
            break;
    }

    printf("Closed %d\n", client_fd);
    close(client_fd);

    Room *room_to_close = ll_get_search(list, search_by_client_fd, &client_fd);

    if (room_to_close != 0)
        for (int i = 0; i < 2; ++i)
            if (room_to_close->clients[i] == client_fd)
                room_to_close->clients[i] = -1;

    return 0;
}

int is_room_offline(Room *room) {
    int offline_clients = 0;
    for (int i = 0; i < 2; ++i)
        if (room->clients[i] == -1)
            offline_clients++;
    return offline_clients == 2;
}

void *gc(void *arg) {
    pthread_detach(pthread_self());
    ll_t *list = arg;
    for (;;) {
        int n = list->len;
        for (int i = 0; i < n; ++i) {
            Room *room = ll_get_n(list, i);
            if (room != 0 && is_room_offline(room)) {
                printf("Room '%s' is waiting\n", room->invite_code);
                sleep(5);
                if (is_room_offline(room)) {
                    printf("Room '%s' had been garbage collected\n", room->invite_code);
                    ll_remove_search(list, search_by_invite_code, room->invite_code);
                }
            }
        }
        sleep(5);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    FILE *urandom = fopen("/dev/random", "r");
    unsigned long seed;
    fgets((char *) &seed, sizeof(seed), urandom);
    fclose(urandom);
    srand(seed);

    int sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in sock_addr;
    bzero(&sock_addr, sizeof(sock_addr));
    sock_addr.sin_family = AF_INET;
    sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    sock_addr.sin_port = htons(8081);

    if (bind(sock_fd, (const struct sockaddr *) &sock_addr, sizeof(sock_addr)) == -1) {
        printf("Bind error!\n");
        return 1;
    }

    if (listen(sock_fd, 10) == -1) {
        printf("Listen error!\n");
        return 1;
    }

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);

    ll_t *list = ll_new(free_room);

    pthread_t gc_id;
    pthread_create(&gc_id, NULL, &gc, list);

    pthread_t tid;
    for (;;) {
        int client_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_len);

        char buff[1024];
        printf("Connected from %s:%hu\n",
               inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff)),
               ntohs(client_addr.sin_port)
        );

        Process_args *args = malloc(sizeof(Process_args));
        args->list = list;
        args->sock_fd = client_fd;

        pthread_create(&tid, NULL, &process_client, args);
    }
}
