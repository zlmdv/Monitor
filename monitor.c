#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <pthread.h>
#include "monitor.h"


void readData (cpuInfo *threadData){
    FILE *file; 
    char buf[256];  
    file = fopen("/proc/stat", "r"); 
    fgets(buf, sizeof(buf), file);
    //initial readings
    sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu", 
    &threadData->user, &threadData->nice, &threadData->system ,&threadData->idle, &threadData->iowait, &threadData->irq, &threadData->softirq, &threadData->steal); 
    fclose(file); 
}

void readMemData(memInfo * memdata){
    FILE *memFile; 
    char buf[256]; 
    memFile = fopen("/proc/meminfo", "r"); 

    //we can read each line individully, because only the first couple lines matter for our purposes 
    // if it was more a loop would be more appropriate 
    fgets(buf, sizeof(buf),  memFile); 
    sscanf(buf, "MemTotal: %lu kB", &memdata->memTotal); 

    fgets(buf, sizeof(buf),  memFile); 
    sscanf(buf, "MemFree: %lu kB", &memdata->memFree); 

    fgets(buf, sizeof(buf),  memFile); 
    sscanf(buf, "MemAvailable: %lu kB", &memdata->memAvailable);

    fgets(buf, sizeof(buf),  memFile); 
    sscanf(buf, "Buffers: %lu kB", &memdata->buffers);

    fgets(buf, sizeof(buf),  memFile); 
    sscanf(buf, "Cached: %lu kB", &memdata->cached);

}

void * calcCpuUsage(void *arg){
    cpuInfo reading1, reading2; 
    uint64_t totalStart, totalEnd, idleStart, idleEnd; 
    uint64_t totalDelta, totalIdle;
    while(1){
        readData(&reading1); 
        sleep(1); 
        readData(&reading2); 

        totalStart = reading1.user + reading1.nice + reading1.system + reading1.idle + reading1.iowait + reading1.irq + reading1.softirq + reading1.steal; 
        totalEnd = reading2.user + reading2.nice + reading2.system + reading2.idle + reading2.iowait + reading2.irq + reading2.softirq + reading2.steal; 
        idleStart = reading1.iowait + reading1.idle; 
        idleEnd = reading2.iowait + reading2.idle; 

        totalDelta = totalEnd - totalStart; 
        totalIdle = idleEnd-idleStart; 

        double cpuUsage  = (100.0 * (totalDelta - totalIdle)/ totalDelta); 

        printf("CPU: %.2f%%\r", cpuUsage); 
        fflush(stdout); 
    }
    return NULL; 
}

void* calcMemUsage(void* arg){
    memInfo memRead; 
    double total, used, free; 
    double usedPercentage; 
    while(1){
        readMemData(&memRead); 
        sleep(1); 
        total = memRead.memTotal / 1024;  //get memory in megabytes
        free = memRead.memFree / 1024; 
        used = (memRead.memTotal - memRead.memFree - memRead.buffers - memRead.cached) / 1024; 
        usedPercentage =  ((double) used / (double)total) * 100;  

        printf("\033[H\033[J");  //terminal escpe code to flush buffers 
        printf("---------------------\r\n"); 
        printf("TOTAL MEMORY (MB): %.2f\r\n", total); 
        printf("USED MEMORY  (MB): %.2f\r\n", used); 
        printf("FREE MEMORY  (MB): %.2f\r\n", free); 
        printf("PERCENT USED       %.3f%%\r\n", usedPercentage); 
        printf("---------------------\r\n"); 
        }
    return NULL; 
    }



int main(){
    pthread_t thread1; 
    pthread_t thread2; 
    if(pthread_create(&thread1, NULL, calcCpuUsage, NULL)!=0){
        return 1; 
    }
    if(pthread_create(&thread2, NULL, calcMemUsage, NULL)!=0){
        return 1; 
    }

    //joining the threads should never happen, but i'll put this here for good style :)    

    if(pthread_join(thread1, NULL)!=0){
        perror("failed to join thread!"); 
        return 1; 
    }

    if(pthread_join(thread2, NULL)!=0){
        perror("failed to join thread!"); 
        return 1; 
    }
    return 0; 
}
 
