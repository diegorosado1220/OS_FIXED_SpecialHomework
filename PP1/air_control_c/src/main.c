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


#include <pthread.h>
#include "functions.h"


#define SHM_NAME "/air_control"

int main() {

  // TODO 1: Call the function that creates the shared memory segment.
  MemoryCreate();

  // TODO 3: Configure the SIGUSR2 signal to increment the planes on the runway
  // by 5.
  struct sigaction sa;
  sa.sa_handler = SigHandler2;
  sigaction(SIGUSR2, &sa, NULL);

  // TODO 4: Launch the 'radio' executable and, once launched, store its PID in
  // the second position of the shared memory block.

   pid_t radio = fork();
    if (radio < 0) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (radio == 0) {
        radio = radioPID;
        if (shm != NULL) {
            shm[1] = getpid();  // pretty sure que [1] es la segunda posicion
        }
    }

  // TODO 6: Launch 5 threads which will be the controllers; each thread will
  // execute the TakeOffsFunction().
  pthread_t threadZ[5];

  for(int i = 0; i < 5; i++){
    pthread_create(&threadZ[i], NULL,TakeOffsFunction, NULL);
  }

  for(int i = 0; i < 5; i++){
    pthread_join(threadZ[i], NULL);
  }
  
}