#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include <sodium.h>
#include "ll.h"

void *malloc_wr(size_t size) {
    void *result = malloc(size);
    if (result != 0)
        return result;
    else {
        printf("Memory allocation error!\n");
        exit(1);
    }
}

void *realloc_wr(void *ptr, size_t size) {
    void *result = realloc(ptr, size);
    if (result != 0)
        return result;
    else {
        printf("Memory reallocation error!\n");
        exit(1);
    }
}

char *gen_invite_code() {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    char *str = malloc_wr(sizeof(char) * 7);

    for (size_t n = 0; n < 7; n++)
        if (n == 3) {
            str[n] = '-';
        } else {
            unsigned int key = randombytes_uniform(sizeof(charset) - 1);
            str[n] = charset[key];
        }

    return str;
}

typedef struct Room {
    int *clients;
    char *invite_code;
} Room;

int send_all(int sock_fd, void *data, int len) {
    unsigned char *data_ptr = (unsigned char *) data;
    int num_sent;

    while (len > 0) {
        num_sent = send(sock_fd, data_ptr, len, 0);
        if (num_sent < 1)
            return 1;
        data_ptr += num_sent;
        len -= num_sent;
    }

    return 0;
}

int send_pkg(int sock_fd, void *body, int body_size, unsigned char id) {
    size_t package_size = sizeof(char) + body_size;
    char package[package_size];
    memcpy(package, &id, sizeof(char));
    memcpy(package + sizeof(char), body, body_size);
    return send_all(sock_fd, package, package_size);
}

int recv_all(int sock_fd, void *data, int len) {
    int num_recv;
    while (len > 0) {
        num_recv = recv(sock_fd, data, len, 0);
        if (num_recv < 1)
            return 1;
        data += num_recv;
        len -= num_recv;
    }

    return 0;
}

Room *create_room(ll_t *list, int client1_fd) {
    Room *new_room = malloc_wr(sizeof(struct Room));
    new_room->clients = malloc_wr(sizeof(int) * 2);
    new_room->clients[0] = client1_fd;
    new_room->clients[1] = -1;

    char *code = gen_invite_code();
    new_room->invite_code = code;

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

int join_room(ll_t *list, int client_fd, char *code, char *address) {
    Room *room = ll_get_search(list, search_by_invite_code, code);
    if (room != NULL)
        for (int i = 0; i < 2; ++i) {
            if (room->clients[i] == -1) {
                room->clients[i] = client_fd;
                printf("Client %s joined to room '%s'\n", address, code);
                return 1;
            }
        }
    return 0;
}

void free_room(void *memory) {
    Room *room = memory;
    free(room->clients);
    free(room->invite_code);
    free(room);
}

typedef struct Process_args {
    ll_t *list;
    int sock_fd;
    char *address;
} Process_args;

int is_room_offline(Room *room) {
    int offline_clients = 0;
    for (int i = 0; i < 2; ++i)
        if (room->clients[i] == -1)
            offline_clients++;
    return offline_clients == 2;
}

int transfer(ll_t *list, int src, int count, unsigned char type) {
    char data[count];
    if (recv_all(src, data, count))
        return 1;

    int second_fd = get_other_client_fd(list, src);
    if (second_fd != -1)
        send_pkg(second_fd, data, count, type);

    return 0;
}

void set_timeout(int sock_fd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;
    setsockopt(sock_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

#define PKG_CREATE_ROOM 0
#define PKG_JOIN_ROOM 1
#define PKG_MOVE 2
#define PKG_CASTLING 3
#define PKG_EN_PASSANT 4
#define PKG_PROMOTION 5
#define PKG_CHAT_MSG 100
#define PKG_STATUS 200

#define PKG_CLIENT_CODE 0
#define PKG_CLIENT_ROOM_FULL 1
#define PKG_CLIENT_MOVE 2
#define PKG_CLIENT_CASTLING 3
#define PKG_CLIENT_EN_PASSANT 4
#define PKG_CLIENT_ROOM_NOT_FOUND 5
#define PKG_CLIENT_JOINED 6
#define PKG_CLIENT_PROMOTION 7
#define PKG_CLIENT_CHAT_MSG 100
#define PKG_CLIENT_STATUS 200

void *process_client(void *arg) {
    pthread_detach(pthread_self());

    Process_args *process_args = arg;
    int client_fd = process_args->sock_fd;
    ll_t *list = process_args->list;
    char *address = process_args->address;
    free(process_args);


    /*
     * Close socket if we won't read
     * enough data in 15s
     */

    set_timeout(client_fd, 15);


    // Filter random connections (spam)
    char *proto = "CHESS_PROTO/1.0";
    size_t proto_len = strlen(proto);
    char client_proto[proto_len];
    if (recv_all(client_fd, &client_proto, proto_len) || strncmp(proto, client_proto, proto_len) != 0) {
        free(address);
        close(client_fd);
        return 0;
    }

    // Process valid client connection
    printf("Connected %s\n", address);

    int error = 0;
    unsigned char package_type;
    while (!error && !recv_all(client_fd, &package_type, sizeof(char)))
        switch (package_type) {
            case PKG_CREATE_ROOM: {
                Room *room = ll_get_search(list, search_by_client_fd, &client_fd);
                if (room == 0) {
                    room = create_room(list, client_fd);

                    // Add terminator to invite code
                    char code[8];
                    strncpy(code, room->invite_code, 7);
                    code[7] = 0;

                    printf("The room '%s' was created by %s\n", code, address);
                }
                error = send_pkg(client_fd, room->invite_code, 7, PKG_CLIENT_CODE);
                if (!error)
                    set_timeout(client_fd, INT_MAX);
                break;
            }
            case PKG_JOIN_ROOM: {
                char code[7];
                if ((error = recv_all(client_fd, &code, 7)))
                    break;

                if (join_room(list, client_fd, code, address)) {
                    if ((error = send_pkg(client_fd, 0, 0, PKG_CLIENT_JOINED)))
                        break;
                    set_timeout(client_fd, INT_MAX);
                    int other_fd = get_other_client_fd(list, client_fd);
                    if (other_fd != -1)
                        send_pkg(other_fd, 0, 0, PKG_CLIENT_ROOM_FULL);
                } else {
                    error = send_pkg(client_fd, 0, 0, PKG_CLIENT_ROOM_NOT_FOUND);
                }
                break;
            }
            case PKG_MOVE:
                error = transfer(list, client_fd, 4, PKG_CLIENT_MOVE);
                break;
            case PKG_EN_PASSANT:
                error = transfer(list, client_fd, 2, PKG_CLIENT_EN_PASSANT);
                break;
            case PKG_CASTLING:
                error = transfer(list, client_fd, 1, PKG_CLIENT_CASTLING);
                break;
            case PKG_PROMOTION:
                error = transfer(list, client_fd, 1, PKG_CLIENT_PROMOTION);
                break;
            case PKG_STATUS: {
                uint16_t rooms_count = list->len;
                uint16_t data = htons(rooms_count);
                error = send_pkg(client_fd, &data, sizeof(uint16_t), PKG_CLIENT_STATUS);
                break;
            }
            case PKG_CHAT_MSG: {
                char *message = 0;
                char next_char;
                int i = 0;
                do {
                    message = (message == 0)
                              ? malloc_wr(sizeof(char))
                              : realloc_wr(message, i + sizeof(char));

                    if (recv_all(client_fd, &next_char, sizeof(char))) {
                        free(message);
                        goto close_connection;
                    }

                    message[i] = next_char;
                    i++;
                } while (next_char != 0);

                int other_fd = get_other_client_fd(list, client_fd);
                if (other_fd != -1)
                    send_pkg(other_fd, message, i, PKG_CLIENT_CHAT_MSG);

                free(message);
                break;
            }
            default:
                error = 0;
        }

    close_connection:

    close(client_fd);
    printf("Closed %s\n", address);
    free(address);

    Room *room_to_close = ll_get_search(list, search_by_client_fd, &client_fd);

    if (room_to_close != 0) {
        char invite_code[7];
        strncpy(invite_code, room_to_close->invite_code, 7);

        for (int i = 0; i < 2; ++i)
            if (room_to_close->clients[i] == client_fd)
                room_to_close->clients[i] = -1;

        // Wait for reconnection
        sleep(5);

        room_to_close = ll_get_search(list, search_by_invite_code, invite_code);
        if (room_to_close != 0 && is_room_offline(room_to_close)) {
            ll_remove_search(list, search_by_invite_code, invite_code);
            printf("The room '%s' was removed\n", invite_code);
        }
    }

    return 0;
}

int get_int_len(int value) {
    int l = 1;
    while (value > 9) {
        l++;
        value /= 10;
    }
    return l;
}

int main() {
    setbuf(stdout, NULL);
    signal(SIGPIPE, SIG_IGN);

    if (sodium_init() == -1) {
        printf("Libsodium init error\n");
        return 1;
    }

    int sock_fd;
    if ((sock_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
        printf("Socket create error!\n");
        return 1;
    }

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


    for (;;) {
        int client_fd;

        if ((client_fd = accept(sock_fd, (struct sockaddr *) &client_addr, &client_addr_len)) == -1)
            continue;

        char buff[1024];
        const char *ip = inet_ntop(AF_INET, &client_addr.sin_addr, buff, sizeof(buff));
        int port = ntohs(client_addr.sin_port);

        size_t len = strlen(ip) + get_int_len(port) + 1;
        char *address = malloc_wr(len * sizeof(char));
        snprintf(address, len, "%s:%d", ip, port);

        Process_args *args = malloc_wr(sizeof(Process_args));
        args->list = list;
        args->sock_fd = client_fd;
        args->address = address;

        pthread_t tid;
        pthread_create(&tid, NULL, &process_client, args);
    }
}
