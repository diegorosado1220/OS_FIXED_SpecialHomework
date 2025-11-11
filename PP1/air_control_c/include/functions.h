#ifndef AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_
#define AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_

void SigHandler2(int signal);

// extern int* shm_ptr;

#define TOTAL_TAKEOFFS 20  // specified afuera
// extern pid_t radioPID;
void MemoryCreate(void);

void* TakeOffsFunction(void* arg);  // para que tambien este en main type shi

#endif  // AIR_CONTROL_C_INCLUDE_FUNCTIONS_H_