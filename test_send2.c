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
int N = 4;                  // number of nodes

int request_number;         // nodes sequence number
int highest_request_number; // highest request number
int outstanding_reply;      // number of outstanding replies
int request_cs;             // flag to request critical section: 1 = request, 0 = not request
int reply_deferred[100];    // reply to node i: 1 = deferred, 0 = not deferred

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

int send_message(msg_type type, int r_number, int senderId) {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message sbuf;
    size_t buf_length;
    key = 1234;
    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    sbuf.type = type;
    sbuf.r_number = r_number;
    sbuf.senderId = senderId;
    buf_length = sizeof(sbuf);
    printf("buf_length is %zu\n", buf_length);
    if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        printf("%d, %d, %d, %d, %zu\n", msqid, sbuf.type, sbuf.r_number, sbuf.senderId, buf_length);
        die("msgsnd");
    } else {
        printf("Message sent\n");
    }
}

int receive_message() {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message rbuf;

    key = 1234;

    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    // We'll receive message type 1
    if (msgrcv(msqid, &rbuf, MAXSIZE, REQUEST, 0) < 0) {
        die("msgrcv");
    }

    printf("%d, %d, %d\n", rbuf.type, rbuf.r_number, rbuf.senderId);
}

int main() {
        printf("Sender process START\n");
        for (int i = 1; i <= 10; i++)
        {
            sleep(1);
            if (i % 2 == 0)
            {
                send_message(REQUEST, i, i*i);
            }
            else
            {
                send_message(REPLY, i, i*i);
            }

        }
        printf("Sender process END\n");
        exit(0);
}
