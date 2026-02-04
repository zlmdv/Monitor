#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>


typedef struct {
    unsigned long user, nice, system, idle, iowait, irq, softirq, steal;
} cpuInfo;

typedef struct {
    unsigned long memTotal, memFree, buffers, cached;
} memInfo;

pthread_mutex_t data_mutex = PTHREAD_MUTEX_INITIALIZER;
double g_cpuUsage = 0.0;
memInfo g_lastMem = {0};

void refreshUI() {
    pthread_mutex_lock(&data_mutex);
    
    double totalMB = g_lastMem.memTotal / 1024.0;
    double usedMB  = (g_lastMem.memTotal - g_lastMem.memFree - g_lastMem.buffers - g_lastMem.cached) / 1024.0;
    double memPerc = (totalMB > 0) ? (usedMB / totalMB) * 100.0 : 0.0;

    printf("\033[H\033[J"); // Clear screen
    printf("========== SYSTEM MONITOR (SYSMON) ==========\n");
    printf("CPU USAGE:    [%.2f%%]\n", g_cpuUsage);
    printf("--------------------------------------------------\n");
    printf("MEM TOTAL:    %.2f MB\n", totalMB);
    printf("MEM USED:     %.2f MB\n", usedMB);
    printf("MEM PERCENT:  %.2f%%\n", memPerc);
    printf("==================================================\n");
    
    pthread_mutex_unlock(&data_mutex);
}

void* calcCpuUsage(void* arg) {
    cpuInfo r1, r2;
    char buf[256];
    while (1) {
        FILE* fp = fopen("/proc/stat", "r");
        if (fp) {
            fgets(buf, sizeof(buf), fp);
            sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &r1.user, &r1.nice, &r1.system, &r1.idle, &r1.iowait, &r1.irq, &r1.softirq, &r1.steal);
            fclose(fp);
        }
        sleep(1);
        fp = fopen("/proc/stat", "r");
        if (fp) {
            fgets(buf, sizeof(buf), fp);
            sscanf(buf, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
                   &r2.user, &r2.nice, &r2.system, &r2.idle, &r2.iowait, &r2.irq, &r2.softirq, &r2.steal);
            fclose(fp);
        }

        unsigned long total1 = r1.user + r1.nice + r1.system + r1.idle + r1.iowait + r1.irq + r1.softirq + r1.steal;
        unsigned long total2 = r2.user + r2.nice + r2.system + r2.idle + r2.iowait + r2.irq + r2.softirq + r2.steal;
        unsigned long idle1  = r1.iowait + r1.idle;
        unsigned long idle2  = r2.iowait + r2.idle;

        pthread_mutex_lock(&data_mutex);
        if (total2 - total1 > 0)
            g_cpuUsage = (100.0 * ((total2 - total1) - (idle2 - idle1)) / (total2 - total1));
        pthread_mutex_unlock(&data_mutex);

        refreshUI();
    }
    return NULL;
}

void* calcMemUsage(void* arg) {
    char label[32];
    unsigned long value;
    memInfo localMem;

    while (1) {
        FILE* fp = fopen("/proc/meminfo", "r");
        if (fp) {
            while (fscanf(fp, "%31s %lu kB", label, &value) != EOF) {
                if (strcmp(label, "MemTotal:") == 0)      localMem.memTotal = value;
                else if (strcmp(label, "MemFree:") == 0)  localMem.memFree = value;
                else if (strcmp(label, "Buffers:") == 0)  localMem.buffers = value;
                else if (strcmp(label, "Cached:") == 0)   { localMem.cached = value; break; }
            }
            fclose(fp);
        }

        pthread_mutex_lock(&data_mutex);
        g_lastMem = localMem; // Copy local struct to global shared state
        pthread_mutex_unlock(&data_mutex);

        refreshUI();
        sleep(1);
    }
    return NULL;
}

int main() {
    pthread_t t1, t2;
    pthread_create(&t1, NULL, calcCpuUsage, NULL);
    pthread_create(&t2, NULL, calcMemUsage, NULL);
    pthread_join(t1, NULL);
    pthread_join(t2, NULL);
    return 0;
}