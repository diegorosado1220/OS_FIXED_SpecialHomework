#ifndef INCLUDE_FUNCTIONS_H_
#define INCLUDE_FUNCTIONS_H_

void SigHandler2(int signal);

extern int *shm; 

#define TOTAL_TAKEOFFS 20 // specified afuera
extern pid_t radioPID;

void* TakeOffsFunction(); // para que tambien este en main type shi

#endif  // INCLUDE_FUNCTIONS_H_