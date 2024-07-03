#ifndef SERVER_H 
#define SERVER_H  
#include <stdint.h>

typedef struct { 
    uint64_t  user, nice, system, idle, iowait, irq, softirq, steal; 
} cpuInfo; 

typedef struct {
    uint64_t memTotal, memFree, memAvailable, buffers, cached; 
}memInfo; 
#endif 