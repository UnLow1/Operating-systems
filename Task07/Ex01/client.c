#include <stdio.h>
#include <stdlib.h>
#include "header.h"
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <signal.h>
#include <unistd.h>
#include <wait.h>
#include <bits/time.h>
#include <time.h>
#include <stdbool.h>

int counter;

void sighandler(int signum);

void timeCheckpoint();

void semaphore(unsigned short num, short op, short flg, struct sembuf *buffer, int semID);

int main(int argc, char *argv[]) {
    if (argc != 3) {
        perror("Wrong arguments! 2 args are needed: N and S");
        exit(-1);
    }

    int N = atoi(argv[1]);  // number of clients
    int S = atoi(argv[2]);  // how many times client should be cut
    counter = 0;

    // getFIFO
    key_t key = ftok(PATH, PROJ_ID);
    int queueID = shmget(key, 0, 0);
    struct MyQueue *myQueue = (struct MyQueue *) shmat(queueID, 0, 0);
//    myQueue->actual_size = 0;

    // getSemaphore
    int semID = semget(key, 0, 0);

    // setSignalHandler
    signal(SIGUSR1, sighandler);
    sigset_t eventMask;
    sigfillset(&eventMask);
    sigdelset(&eventMask, SIGUSR1);
    sigset_t blockMask;
    sigemptyset(&blockMask);
    sigaddset(&blockMask, SIGUSR1);
    sigprocmask(SIG_BLOCK, &blockMask, NULL);

    bool inSalon = false;
    int i;
    pid_t child_pid = NULL;
    for (i = 0; i < N; i++) {
        child_pid = fork();
        if (child_pid == 0) {
            // child process
            struct sembuf buffer;
            pid_t pid = getpid();
            timeCheckpoint();
            printf("Client %d starting\n", pid);
            while (counter < S) {

                semaphore(1, -1, 0, &buffer, semID);

                int barberActive = semctl(semID, 0, GETVAL);
                if (!barberActive) {

                    semaphore(0, 1, 0, &buffer, semID);
                    timeCheckpoint();
                    printf("Client %d is waking up barber\n", pid);
                    inSalon = true;

                    semaphore(0, 1, 0, &buffer, semID);
                    myQueue->chair = pid;
                } else {
                    if (myQueue->max_size > myQueue->actual_size) {
                        // take a seat in a myQueue
                        myQueue->queue[myQueue->actual_size] = pid;
                        myQueue->actual_size++;
                        inSalon = true;
                        timeCheckpoint();
                        printf("Client %d is in queue\n",pid);
                    } else {
                        // Barber's studio is full
                        timeCheckpoint();
                        printf("Queue is full. Client %d is leavnig Barber.\n", pid);
		        sleep(1);
                    }
                }


                semaphore(1, 1, 0, &buffer, semID);

                if (inSalon) {
                    sigsuspend(&eventMask);
                    timeCheckpoint();
                    printf("Client %d got cut. Client is leaving Barber.\n", pid);
		    inSalon = false;
                }

            }
            timeCheckpoint();
            printf("Client %d ending work\n", pid);
            shmdt(myQueue);
            exit(1);
        }
    }
    int status;
    waitpid(child_pid, &status, 0);
    shmdt(myQueue);
    return 0;
}

void sighandler(int signum) {
    if (signum == SIGUSR1)
        counter++;
    else if (signum == SIGINT) {
        printf("\nGot SIGINT, ending work.\n");
        exit(1);
    }
}

void timeCheckpoint() {
    struct timespec real_time;
    clock_gettime(CLOCK_MONOTONIC, &real_time);
    printf("timeCheckpoint %lld    ", (long long) (real_time.tv_sec * 1000000 + real_time.tv_nsec / 1000));
}

void semaphore(unsigned short num, short op, short flg, struct sembuf *buffer, int semID) {
    buffer->sem_num = num;
    buffer->sem_op = op;
    buffer->sem_flg = flg;
    semop(semID, buffer, 1);
}
