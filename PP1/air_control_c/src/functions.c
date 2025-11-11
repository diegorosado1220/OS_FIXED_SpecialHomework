#define _XOPEN_SOURCE 500
#include "../include/functions.h"
// #define _POSIX_C_SOURCE 200809L
// #define TOTAL_TAKEOFFS 20

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define SHM_NAME "/air_control"
#define SHM_ELEMENTS 3
#define SHM_BLOCK_SIZE (SHM_ELEMENTS * sizeof(int))
int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int shm_1;
int* shm_ptr = NULL;

pthread_mutex_t state_lock;

pthread_mutex_t runway1_lock;
pthread_mutex_t runway2_lock;

void MemoryCreate() {
  shm_1 = shm_open(SHM_NAME, O_CREAT | O_RDWR, 0666);
  ftruncate(shm_1, SHM_BLOCK_SIZE);
  shm_ptr =
      mmap(NULL, SHM_BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_1, 0);
  if (shm_ptr == MAP_FAILED) {
    perror("mmap failed");
    exit(EXIT_FAILURE);
  }
  shm_ptr[0] = getpid();  // store the pid of the current process in the first
                          // position of the memory block
  shm_ptr[1] = 0;
  shm_ptr[2] = 0;
}

void SigHandler2(int signal) {
  if (signal == SIGUSR2) {
    planes += 5;
  }
}

void* TakeOffsFunction(void* arg) {
  pthread_mutex_init(&state_lock, NULL);
  pthread_mutex_init(&runway1_lock, NULL);
  pthread_mutex_init(&runway2_lock, NULL);

  while (1) {
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS) {
      pthread_mutex_unlock(&state_lock);
      break;
    }
    pthread_mutex_unlock(&state_lock);
    int krrnttrack = 0;
    if (pthread_mutex_trylock(&runway1_lock) == 0) {
      krrnttrack = 1;
    } else if (pthread_mutex_trylock(&runway2_lock) == 0) {
      krrnttrack = 2;
    } else {
      usleep(1000);
      continue;
    }

    pthread_mutex_lock(&state_lock);

    if (planes > 0 && total_takeoffs < TOTAL_TAKEOFFS) {
      planes--;
      takeoffs++;
      total_takeoffs++;

      if (takeoffs == 5) {
        kill(shm_ptr[1], SIGUSR1);
        takeoffs = 0;
      }

      int just_reached_limit = (total_takeoffs == TOTAL_TAKEOFFS);
      pthread_mutex_unlock(&state_lock);

      sleep(1);

      if (krrnttrack == 1) {
        pthread_mutex_unlock(&runway1_lock);
      } else {
        pthread_mutex_unlock(&runway2_lock);
      }

      if (just_reached_limit) {
        kill(shm_ptr[1], SIGTERM);
        munmap(shm_ptr, SHM_BLOCK_SIZE);
        close(shm_1);
        return NULL;
      }

    } else {
      pthread_mutex_unlock(&state_lock);
      if (krrnttrack == 1) {
        pthread_mutex_unlock(&runway1_lock);
      } else {
        pthread_mutex_unlock(&runway2_lock);
      }
      usleep(1000);
    }
  }
  return NULL;
}