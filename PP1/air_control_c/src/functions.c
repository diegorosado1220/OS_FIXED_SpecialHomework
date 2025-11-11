#define _XOPEN_SOURCE 500
#include "../include/functions.h"
// #define _POSIX_C_SOURCE 200809L
// #define TOTAL_TAKEOFFS 20

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
#include <time.h>

#define SHM_NAME "/air_control"
#define SHM_ELEMENTS 3
#define SHM_BLOCK_SIZE (SHM_ELEMENTS * sizeof(int))
int planes = 0;
int takeoffs = 0;
int total_takeoffs = 0;

int shm_1;
int* shm_ptr = NULL;

pthread_mutex_t state_lock = PTHREAD_MUTEX_INITIALIZER;

// pthread_mutex_t* KrrntTrack;
// int KrrntTrack = 0;
pthread_mutex_t runway1_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t runway2_lock = PTHREAD_MUTEX_INITIALIZER;

void MemoryCreate() {
  // TODO2: create the shared memory segment, configure it and store the PID of
  // the process in the first position of the memory block.
  // shm_unlink(SHM_NAME);
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

  // close(shm_1);
  // shm = addr;      //gpt
  // shm[0] = getpid();
}

void SigHandler2(int signal) {
  if (signal == SIGUSR2) {
    planes += 5;
  }
}

void* TakeOffsFunction(void* arg) {
  // TODO: implement the logic to control a takeoff thread
  //    Use a loop that runs while total_takeoffs < TOTAL_TAKEOFFS
  //    Use runway1_lock or runway2_lock to simulate a locked runway
  //    Use state_lock for safe access to shared variables (planes,
  //    takeoffs, total_takeoffs)
  //    Simulate the time a takeoff takes with sleep(1)
  //    Send SIGUSR1 every 5 local takeoffs
  //    Send SIGTERM when the total takeoffs target is reached
  while (1) {
    pthread_mutex_lock(&state_lock);
    if (total_takeoffs >= TOTAL_TAKEOFFS) {
        pthread_mutex_unlock(&state_lock);
        break;
    }
    //------------------------------------------- 
    // gpt fix or smth
    pthread_mutex_unlock(&state_lock);
    int KrrntTrack = 0;
     if (pthread_mutex_trylock(&runway1_lock) == 0) {
      KrrntTrack = 1;
    } else if (pthread_mutex_trylock(&runway2_lock) == 0) {
      KrrntTrack = 2;
    } else {
      usleep(1000);
      continue;
    }

    // 3) Zona crítica: actualizar estado si hay aviones
    pthread_mutex_lock(&state_lock);

    if (planes > 0 && total_takeoffs < TOTAL_TAKEOFFS) {
      planes--;
      takeoffs++;
      total_takeoffs++;

      printf("planes=%d, total_takeoffs=%d\n", planes, total_takeoffs);

      // Avisar cada 5 despegues al RADIO y al GROUND
      if (takeoffs == 5) {
        kill(shm_ptr[1], SIGUSR1);  // radio
        // kill(shm_ptr[2], SIGUSR1);  // ground
        takeoffs = 0;
      }

      // ¿Acabamos de llegar EXACTAMENTE al límite?
      int just_reached_limit = (total_takeoffs == TOTAL_TAKEOFFS);

      pthread_mutex_unlock(&state_lock);

      // 4) Simular tiempo de despegue
      sleep(1);

      // 5) Liberar la pista
      if (KrrntTrack == 1)
        pthread_mutex_unlock(&runway1_lock);
      else
        pthread_mutex_unlock(&runway2_lock);

      // 6) Si este hilo ha hecho el despegue número 20,
      //    él es el ÚNICO que manda SIGTERM y cierra memoria
      if (just_reached_limit) {
        kill(shm_ptr[1], SIGTERM);              // radio
        munmap(shm_ptr, SHM_BLOCK_SIZE);        // cerrar mapping
        close(shm_1);                           // cerrar descriptor
        return NULL;                            // salir del hilo
      }

    } else {
      // No hay aviones o ya se alcanzó el objetivo
      pthread_mutex_unlock(&state_lock);

      if (KrrntTrack == 1)
        pthread_mutex_unlock(&runway1_lock);
      else
        pthread_mutex_unlock(&runway2_lock);

      usleep(1000);
    }
    //------------------------------------------------------------
    // pthread_mutex_unlock(&state_lock);
    // int KrrntTrack = 0;
    // if (pthread_mutex_trylock(&runway1_lock) == 0) {
    //   KrrntTrack = 1;
    //   pthread_mutex_lock(&state_lock);
    //   if (planes > 0) {
        
    //     // KrrntTrack = 1;
    //     planes--;
    //     takeoffs++;
    //     total_takeoffs++;
    //     printf("[AIR] planes=%d, total_takeoffs=%d\n", planes, total_takeoffs);
    //     if (takeoffs == 5) {
    //       kill(shm_ptr[1], SIGUSR1);
    //       kill(shm_ptr[2], SIGUSR1);
    //       takeoffs = 0;
    //     }
    //   }
    //   pthread_mutex_unlock(&state_lock);
    // } else if (pthread_mutex_trylock(&runway2_lock) == 0) {
    //   KrrntTrack = 2;
    //   pthread_mutex_lock(&state_lock);
    //   if (planes > 0) {
        
    //     // KrrntTrack = 2;
    //     planes--;
    //     takeoffs++;
    //     total_takeoffs++;
    //     printf("[AIR] planes=%d, total_takeoffs=%d\n", planes, total_takeoffs);
    //     if (takeoffs == 5) {
    //       kill(shm_ptr[1], SIGUSR1);
    //       kill(shm_ptr[2], SIGUSR1);
    //       takeoffs = 0;
    //     }
    //   }
    //   pthread_mutex_unlock(&state_lock);
    // } else {
    //   usleep(
    //       1000);  // if unsuccessful in locking runway, sleep for a short time
    //   continue;
    // }

    // sleep(1);

    // if (KrrntTrack == 1) {
    //   pthread_mutex_unlock(&runway1_lock);
    // } else if (KrrntTrack == 2){
    //   pthread_mutex_unlock(&runway2_lock);
    // }
  }

  // kill(shm_ptr[1], SIGTERM);  // send SIGTERM to radio process
  // munmap(shm_ptr, SHM_BLOCK_SIZE);
  // close(shm_1);
  // shm_unlink(SHM_NAME);
  return NULL;
}