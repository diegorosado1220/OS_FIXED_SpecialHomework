#define _XOPEN_SOURCE 500
#include "../include/functions.h"

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/time.h>

#define SHM_NAME "/air_control"
#define SHM_ELEMENTS 3
#define SHM_BLOCK_SIZE (SHM_ELEMENTS * sizeof(int))


#define PLANES_LIMIT 20
int planes = 0;
int takeoffs = 0;
int traffic = 0;

int queued_planes = 0;
int fd;
int* shm_ptr;
void SigHandler1(int signal);
void alarm_handler(int sig);


void Traffic(int signum) {
  // TODO:
  // Calculate the number of waiting planes.
  // Check if there are 10 or more waiting planes to send a signal and increment
  // planes. Ensure signals are sent and planes are incremented only if the
  // total number of planes has not been exceeded.
  (void)signum;

  queued_planes = planes - takeoffs;
  
  if (queued_planes >= 10){
    printf("RUNWAY OVERLOADED\n");
    return;
  }
  if (planes < PLANES_LIMIT){
    if(planes == 16){
      planes += 4;
    } else if(planes == 17){
      planes += 3;
    } else if(planes == 18){
      planes += 2;
    } else if(planes == 19){
      planes += 1;
    } else {
      planes += 5;
    }
    // kill(shm_ptr[1], SIGUSR2);
    int radio_pid = shm_ptr[1];
    if (radio_pid > 0) {
      kill(radio_pid, SIGUSR2);
    }
  }
  return;
}

int main(int argc, char* argv[]) {
  // TODO:
  // 1. Open the shared memory block and store this process PID in position 2
  //    of the memory block.
  pid_t ground_control_pid;
  ground_control_pid = getpid();
  fd = shm_open(SHM_NAME, O_RDWR, 0666);
  if (fd == -1) {
      perror("shm_open failed in child");
      exit(EXIT_FAILURE);
    }
  shm_ptr = mmap(NULL, SHM_BLOCK_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);
  if (shm_ptr == MAP_FAILED) {
      perror("mmap failed in child");
      exit(EXIT_FAILURE);
    }
  shm_ptr[2] = ground_control_pid;
  

    // int radio_pid = shm_ptr[1];
  // 3. Configure SIGTERM and SIGUSR1 handlers
  //    - The SIGTERM handler should: close the shared memory, print
  //      "finalization of operations..." and terminate the program.
  //    - The SIGUSR1 handler should: increase the number of takeoffs by 5.

  struct sigaction sa1 = {0};
  sa1.sa_handler = SigHandler1;
  // sigaction(SIGUSR1, &sa1, NULL);
  // sigaction(SIGTERM, &sa1, NULL);

  if (sigaction(SIGUSR1, &sa1, NULL) == -1) {
    perror("sigaction SIGUSR1");
    exit(EXIT_FAILURE);
}
if (sigaction(SIGTERM, &sa1, NULL) == -1) {
    perror("sigaction SIGTERM");
    exit(EXIT_FAILURE);
}

  struct sigaction sa3 = {0};
  sa3.sa_handler = alarm_handler;
  // sigaction(SIGALRM, &sa3, NULL);

  if (sigaction(SIGALRM, &sa3, NULL) == -1) {
    perror("sigaction SIGALRM");
    exit(EXIT_FAILURE);
}

  // 2. Configure the timer to execute the Traffic function.
  struct itimerval timer;

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 500000;

  // Then every 1 second:
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 500000;


  if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        return 1;
    }
  while(1){
    pause();
  }
  return 0;
}

void SigHandler1(int signal){

  if (SIGTERM == signal) {
    munmap(shm_ptr, SHM_BLOCK_SIZE);
    close(fd);
    // printf("Terminating  ground cotrl\n");
    exit(EXIT_SUCCESS);
  } else if (SIGUSR1 == signal) {
    takeoffs += 5;
    // planes -=5;
    // if (planes < 0) {
    //   planes = 0;
    // }
  }
}
void alarm_handler(int sig) {
  Traffic(sig);
}
