#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <strings.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <pthread.h>
#include <memory.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#define MSG_SIZE 256
#define MAX_QUEUES 2
#define MAX_THREADS 2

key_t keys[MAX_QUEUES];
int qids[MAX_QUEUES];
bool running;
int state;

enum {
    STATE_RECV,
    STATE_SEND,
    STATE_RESPONSE_HANDLER
};

typedef struct {
    long type;
    struct sockaddr_in addr;
    char buf[MSG_SIZE];
} msgbuf;

void signal_handler(int signo)
{
    switch (signo) {
        case SIGINT:
        case SIGTERM:
            running = false;
            break;
        default:
            exit(-1);
    }
}

void *worker(void *arg)
{
    time_t tm;
    msgbuf req;
    while (running) {
        msgrcv(qids[0], &req, sizeof(req) - (sizeof(long)), 1L, 0);
        tm = time(0);
        snprintf(req.buf, MSG_SIZE, "time - %s", ctime(&tm));
        msgsnd(qids[1], &req, sizeof(req) - (sizeof(long)), 0);
    }
    pthread_exit(NULL);
}

void server(unsigned short port)
{
    pthread_t th[MAX_THREADS];
    struct sockaddr_in serverAddr;
    struct timeval timeout;
    msgbuf req;
    int serverSock, i;
    ssize_t result;
    ssize_t nbytes;

    timeout.tv_sec = 1;
    timeout.tv_usec = 0;

    bzero(&req, sizeof(req));

    for (i = 0; i < MAX_QUEUES; ++i) {
        keys[i] = ftok(".", 'a' + i);
        qids[i] = msgget(keys[i], 0666 | IPC_CREAT);
    }

    for (i = 0; i < MAX_THREADS; ++i) {
        pthread_create(&th[i], NULL, worker, NULL);
    }

    serverSock = socket(AF_INET, SOCK_DGRAM, 0);

    setsockopt(serverSock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    serverAddr.sin_port = htons(port);
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_family = AF_INET;

    bind(serverSock, (struct sockaddr *)&serverAddr, sizeof(serverAddr));

    running = true;

    state = STATE_RECV;

    socklen_t socklen;

    while (running) {

        switch (state) {
            case STATE_RECV:
                printf("STATE_RECV\n");
                nbytes = recvfrom(serverSock, req.buf, MSG_SIZE, 0, (struct sockaddr *) &req.addr, &socklen);
                if (nbytes < 0 && errno == EWOULDBLOCK)
                    state = STATE_SEND;
                else if (nbytes < 0) {
                    running = false;
                } else {
                    state = STATE_RESPONSE_HANDLER;
                }
                break;
            case STATE_SEND:
                printf("STATE_SEND\n");
                result = msgrcv(qids[1], &req, sizeof(req) - (sizeof(long)), 1L, MSG_NOERROR | IPC_NOWAIT);
                if (result < 0 && errno == ENOMSG) {
                    printf("empty queue\n");
                    state = STATE_RECV;
                    break;
                }
                socklen = sizeof(req.addr);
                sendto(serverSock, req.buf, MSG_SIZE, 0, (struct sockaddr *) &req.addr, socklen);
                state = STATE_RECV;
                break;
            case STATE_RESPONSE_HANDLER:
                printf("STATE_RESPONSE_HANDLER\n");
                req.type = 1L;
                msgsnd(qids[0], &req, sizeof(req) - (sizeof(long)), IPC_NOWAIT);
                state = STATE_RECV;
                break;
            default:
                break;
        }

    }

    for (i = 0; i < MAX_THREADS; ++i) {
        pthread_join(th[i], NULL);
    }

    for (i = 0; i < MAX_QUEUES; ++i) {
        msgctl(qids[i], IPC_RMID, 0);
    }

    close(serverSock);
}

void client(unsigned short port)
{
    int sock;
    struct sockaddr_in addr;
    char buf[MSG_SIZE];
    socklen_t socklen;

    sock = socket(AF_INET, SOCK_DGRAM, 0);

    bzero(&buf, MSG_SIZE);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(port);

    socklen = sizeof(addr);

    running = true;

    while (running) {
        sleep(1);
        struct sockaddr_in clientAddr;
        sendto(sock, buf, MSG_SIZE, 0, (struct sockaddr *)& addr, socklen);

        recvfrom(sock, buf, MSG_SIZE, 0, (struct sockaddr *)& clientAddr, (socklen_t *) sizeof(clientAddr));
        printf("%s", buf);
        bzero(&buf, MSG_SIZE);
    }

    close(sock);
}

int main(void)
{
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    char command[256];
    fgets(command, 256, stdin);
    if (strcmp(command, "server") > 0) {
        printf("server\n");
        server(8005);
    } else {
        printf("client\n");
        client(8000);
    }
    return 0;
}
