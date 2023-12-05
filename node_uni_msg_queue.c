#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/stat.h>

#define MAXSIZE     128
#define me 1

typedef struct SharedData {
    int N;                      // number of nodes
    int request_number;         // nodes sequence number
    int highest_request_number; // highest request number
    int outstanding_reply;      // number of outstanding replies
    int request_cs;             // flag to request critical section: 1 = request, 0 = not request
    int reply_deferred[10];    // reply to node i: 1 = deferred, 0 = not deferred
    sem_t mutex;                // for mutual exclusion to shared variables
    sem_t wait_sem;             // used to wait for all requests
} SharedData;

// message types
typedef enum {REQUEST, REPLY, PRINT, NEW_NODE} msg_type;

// message structure
typedef struct {
    long mtype;             // node id, each node only receives message sent to itself
    msg_type type;          // REQUEST, REPLY, PRINT, NEW_NODE
    int to;
    int req_value;           // request number
    int from;           // sender node id
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
void print_to_server(char *s) {
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
    // printf("buf_length is %zu\n", buf_length);
    if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        die("msgsnd");
    } else {
        printf("Message sent\n");
    }
}

Message receive_message(long mtype) {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message rbuf;

    key = 1234;

    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }

    if (msgrcv(msqid, &rbuf, MAXSIZE, mtype, 0) < 0) {
        die("msgrcv");
    }
    return rbuf;
}

int send_message(long receiverId, msg_type type, int req_value, int from) {
    int msqid;
    int msgflg = IPC_CREAT | 0666;
    key_t key;
    Message sbuf;
    size_t buf_length;
    key = 1234;
    if ((msqid = msgget(key, msgflg)) < 0) {
        die("msgget");
    }
    sbuf.mtype = receiverId;
    sbuf.type = type;
    sbuf.req_value = req_value;
    sbuf.from = from;
    buf_length = sizeof(sbuf)-sizeof(long);
    if (msgsnd(msqid, &sbuf, buf_length, IPC_NOWAIT) < 0) {
        die("msgsnd");
    } else {
        printf("Message sent to %ld, type is\n", sbuf.mtype, sbuf.type);
    }
}

int main() {
    int shm_id;
    SharedData *shared_data;
    // printf("size of SharedData is %lu\n", sizeof(SharedData));
    shm_id = shmget(IPC_PRIVATE, sizeof(SharedData), S_IRUSR | S_IWUSR);
    shared_data = shmat(shm_id, NULL, 0);
    // printf("shared_data address is %p\n", shared_data);
    // printf("shared_data->N address is %p\n", &shared_data->N);
    // printf("shared_data->request_number address is %p\n", &shared_data->request_number);

    shared_data->N = 4;
    // printf("shared_data->N address is %d\n", shared_data->N);

    shared_data->request_number = 0;
    shared_data->highest_request_number = 0;
    shared_data->outstanding_reply = 0;
    shared_data->request_cs = 0;
    // printf("DEBUG\n");

    sem_init(&shared_data->mutex, 1, 1);
    sem_init(&shared_data->wait_sem, 1, 0);
    // printf("DEBUG\n");

    pid_t pid;
    pid = fork();
    if (pid == 0) {
        // Receive message
        while (1) {
            Message ret = receive_message(me);
            printf("Received message sent to %ld, type: %d, request number: %d, from: %d\n", ret.mtype, ret.type, ret.req_value, ret.from);
            if (ret.type == REQUEST) {
                int k = ret.req_value;
                int i = ret.from;
                int defer_it = 0;
                if (k > shared_data->highest_request_number) {
                    shared_data->highest_request_number = k;
                }
                sem_wait(&shared_data->mutex);
                defer_it = (shared_data->request_cs) && ((k > shared_data->request_number) || ((k == shared_data->request_number) && (i > me)));
                sem_post(&shared_data->mutex);
                if (defer_it) {
                    printf("Node 1 deferring REPLY to %d\n", i);
                    shared_data->reply_deferred[i] = 1;
                } else {
                    send_message(i, REPLY, -1, me);
                }
            }
            else if (ret.type == REPLY) {
                printf("Node 1 received REPLY from %d\n", ret.from);
                shared_data->outstanding_reply -= 1;
                sem_post(&shared_data->wait_sem);
            }
            
        }
        
    } else {
        // Request process
        sleep(1);
        while (1) {
            printf("Node 1 requesting critical section\n");

            sem_wait(&shared_data->mutex);
            shared_data->request_cs = 1;
            // printf("DEBUG\n");
            shared_data->request_number = shared_data->highest_request_number + 1;
            sem_post(&shared_data->mutex);

            shared_data->outstanding_reply = shared_data->N - 1;
            printf("Request Number is %d\n", shared_data->request_number);
            printf("%d replies needed\n", shared_data->outstanding_reply);
            for (int i = 1; i <= shared_data->N; i++) {
                if (i != me) {
                    printf("Node 1 sending REQUEST to %d\n", i);
                    send_message(i, REQUEST, shared_data->request_number, me);
                }
            }

            printf("Node 1 waiting for replies\n");
            while (shared_data->outstanding_reply > 0) {
                sem_wait(&shared_data->wait_sem);
            }

            // ENTER Critical section
            printf("Node 1 entering critical section\n");
            print_to_server("########## START OUTPUT FOR NODE 1 ###############");
            for (int i = 0; i < 4; i++) {
                sleep(1);
                print_to_server("Node 1 printing: Hello world");
            }
            print_to_server("----------- END OUTPUT FOR NODE 1 ----------------");
            // LEAVE Critical section
            printf("Node 1 leaving critical section\n");

            shared_data->request_cs = 0;
            for (int i = 1; i <= shared_data->N; i++) {
                printf("DEBUG: shared_data->reply_deferred[%d] is %d\n", i, shared_data->reply_deferred[i]);
                if (shared_data->reply_deferred[i]) {
                    printf("Node 1 sending deferred REPLY to %d\n", i);
                    send_message(i, REPLY, -1, me);
                    shared_data->reply_deferred[i] = 0;
                }
            }
        }
    }
    return 0;
}
