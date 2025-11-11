#define _XOPEN_SOURCE 500
#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/unistd.h>
#include <sys/wait.h>

#include "../include/functions.h"

#define SHM_NAME "/air_control"
#define SHM_ELEMENTS 3
#define SHM_BLOCK_SIZE (SHM_ELEMENTS * sizeof(int))

int main() {
  MemoryCreate();

  struct sigaction sa = {0};
  sa.sa_handler = SigHandler2;

  if (sigaction(SIGUSR2, &sa, NULL) == -1) {
    perror("sigaction SIGUSR2");
    exit(EXIT_FAILURE);
  }

  pid_t radio_pid, air_control_pid;
  air_control_pid = getpid();
  radio_pid = fork();

  if (radio_pid < 0) {
    perror("fork failure");
    exit(EXIT_FAILURE);
  } else if (radio_pid == 0) {  // Child process
    radio_pid = getpid();
    int fd = shm_open(SHM_NAME, O_RDWR, 0666);  // open existing shared memory
    if (fd == -1) {
      perror("shm_open failed in child");
      exit(EXIT_FAILURE);
    }
    int* shm_ptr = mmap(NULL, SHM_BLOCK_SIZE, PROT_READ | PROT_WRITE,
                        MAP_SHARED, fd, 0);  // map shared memory
    if (shm_ptr == MAP_FAILED) {
      perror("mmap failed in child");
      exit(EXIT_FAILURE);
    }
    shm_ptr[1] = radio_pid;  // store the pid of the radio process in second
                             // position of shared memory
    execl("./radio", "radio", SHM_NAME,
          NULL);  // transforming child into radio
  } else {
    pthread_t threadZ[5];

    for (int i = 0; i < 5; i++) {
      pthread_create(&threadZ[i], NULL, TakeOffsFunction, NULL);
    }

    for (int i = 0; i < 5; i++) {
      pthread_join(threadZ[i], NULL);
    }

    shm_unlink(SHM_NAME);
  }
  return 0;
}