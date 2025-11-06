#define _XOPEN_SOURCE 700
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <sys/stat.h>

#include "functions.h"

#include <pthread.h>

#define SHM_NAME "/air_control"

int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int *shm = NULL;

pthread_mutex_t state_lock;

pthread_mutex_t *KrrntTrack;
pthread_mutex_t runway1_lock;
pthread_mutex_t runway2_lock;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  int fd = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(fd, 1024);
  void *shm_ptr = mmap(NULL, 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
  void *addr = (void *)shm_ptr; // gpt

  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    exit(1);
  }

  close(fd);

  shm = addr;      //gpt
  shm[0] = getpid();
}

void SigHandler2(int signal) {
  planes += 5;
}

void* TakeOffsFunction() {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
  while(total_takeoffs < TOTAL_TAKEOFFS){
    if (pthread_mutex_trylock(&runway1_lock) == 0) {
      KrrntTrack = &runway1_lock;
    } else if(pthread_mutex_trylock(&runway2_lock) == 0){  // layout idea from gpt (skull emoji)
      KrrntTrack = &runway2_lock; // 
    } else {
      continue;
    }

    if(planes > 0){
      planes--;
      takeoffs++;
      total_takeoffs++;

      if(takeoffs == 5){
        kill(radioPID, SIGUSR1); // radioPID es un dummy en lo que se me ocurre algo
        takeoffs = 0;
      }
    }

    pthread_mutex_unlock(&state_lock);

    sleep(1);
    pthread_mutex_unlock(&KrrntTrack);
  }

  kill(radioPID, SIGTERM);
  shm_unlink(SHM_NAME);
  return 0;
}