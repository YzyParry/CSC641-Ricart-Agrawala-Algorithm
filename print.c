#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAXSIZE     128
void die(char *s) {
    perror(s);
    exit(1);
}

typedef struct {
    long mtype;
    char data[MAXSIZE];
} msgbuf;

int main() {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    msgbuf rbuf;
    key = 5678;
    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    // We'll receive message type 1
    printf("Print server is ready to receive messages.\n");
    while (1)
    {
        if (msgrcv(msqid, &rbuf, MAXSIZE, 0, 0) < 0) {
            die("msgrcv");
        }
        printf("%s\n", rbuf.data);
    }
}