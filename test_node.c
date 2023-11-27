#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAXSIZE     128

void die(char *s) {
    perror(s);
    exit(1);
}

// Shared variables
#define NUM_REPLY 0
#define N 4                 // number of nodes

int request_number;         // nodes sequence number
int highest_request_number; // highest request number
int outstanding_reply;      // number of outstanding replies
int request_cs;             // flag to request critical section: 1 = request, 0 = not request
int reply_deferred[N];      // reply to node i: 1 = deferred, 0 = not deferred

sem_t mutex;                // for mutual exclusion to shared variables
sem_t wait_sem;             // used to wait for all requests

// message types
typedef enum {REQUEST, REPLY} msg_type;

// message structure
typedef struct {
    msg_type type;          // REQUEST or REPLY
    int r_number;           // request number
    int senderId;           // sender node id
} Message;

int send_message(int type, int r_number, int senderId) {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message sbuf;
    size_t buf_length;
    key = 1234;
    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    sbuf.type = REQUEST;
    sbuf.r_number = r_number;
    sbuf.senderId = senderId;
    buf_length = sizeof(sbuf);
    printf("buf_length is %d\n", buf_length);
    if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        printf("%d, %d, %d, %d, %zu\n", msqid, sbuf.type, sbuf.r_number, sbuf.senderId, buf_length);
        die("msgsnd");
    } else {
        printf("Message sent\n");
    }
}

int receive_REQUEST() {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message rbuf;

    key = 1234;

    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    // We'll receive message type 1
    if (msgrcv(msqid, &rbuf, MAXSIZE, 0, 0) < 0) {
        die("msgrcv");
    }

    printf("%d, %d, %d\n", rbuf.type, rbuf.r_number, rbuf.senderId);
}

int receive_REPLY() {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message rbuf;

    key = 1234;

    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    // We'll receive message type 1
    if (msgrcv(msqid, &rbuf, MAXSIZE, 0, 0) < 0) {
        die("msgrcv");
    }

    printf("%d, %d, %d\n", rbuf.type, rbuf.r_number, rbuf.senderId);
}

int main() {
    pid_t pid1, pid2;
    pid1 = fork();
    if (pid1 == 0) {
        // child process
        while (1) {
            printf("Parent process waiting for REPLY message\n");
            receive_REPLY();
        }
        exit(0);
    } else if (pid1 < 0) {
        // fork failed
        printf("Fork failed\n");
        exit(1);
    } else {
        // parent process
        while (1) {
            printf("Parent process waiting for REQUEST message\n");
            receive_REQUEST();
        }
        exit(0);
    }
}
