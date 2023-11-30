#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>

#define MAXSIZE     128

int N;                      // number of nodes

int request_number;         // nodes sequence number
int highest_request_number; // highest request number
int outstanding_reply;      // number of outstanding replies
int request_cs;             // flag to request critical section: 1 = request, 0 = not request
int reply_deferred[100];    // reply to node i: 1 = deferred, 0 = not deferred

sem_t mutex;                // for mutual exclusion to shared variables
sem_t wait_sem;             // used to wait for all requests

// message types
typedef enum {_,REQUEST, REPLY, PRINT, NEW_NODE} msg_type;

// message structure
typedef struct {
    long mtype;             // node id, each node only receives message sent to itself
    msg_type type;          // REQUEST, REPLY, PRINT, NEW_NODE
    int r_number;           // request number
    int senderId;           // sender node id
} Message;

typedef struct {
    long mtype;
    char data[MAXSIZE];
} msgbuf;

void die(char *s) {
    perror(s);
    exit(1);
}

// Send message to print server
void print(char *s) {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    msgbuf sbuf;
    size_t buf_length;
    key = 5678;
    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    sbuf.mtype = 1;
    strcpy(sbuf.data, s);
    buf_length = sizeof(sbuf)-sizeof(long);
    printf("buf_length is %zu\n", buf_length);
    if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        die("msgsnd");
    } else {
        printf("Message sent\n");
    }
}

int main() {
    print("hello");
    print("this is hacker");
    return 0;
}
