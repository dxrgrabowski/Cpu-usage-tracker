#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>

#define NUM_CORES 4
#define NUM_THREADS 5
#define READ_DELAY 30000 
typedef struct {
    unsigned long user;
    unsigned long nice;
    unsigned long system;
    unsigned long idle;
    unsigned long iowait;
    unsigned long irq;
    unsigned long softirq;
    unsigned long steal;
} CPUData;

typedef struct {
    CPUData current;
    CPUData previous;
    double usage;
} CPUUsage;

// Global variables
CPUUsage cpuUsage[NUM_CORES];
pthread_mutex_t cpuUsageMutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t readerUpdated = PTHREAD_COND_INITIALIZER;
pthread_cond_t analyzerUpdate = PTHREAD_COND_INITIALIZER;
int threadsRunning = 1;

// Function to read CPU data from /proc/stat
void readCPUData(CPUData* cpuData) {
    FILE* file = fopen("/proc/stat", "r");
     if (file == NULL) {
        printf("Error: Failed to open /proc/stat file.\n");
    }

    fscanf(file, "cpu %lu %lu %lu %lu %lu %lu %lu %lu",
        &cpuData->user, &cpuData->nice, &cpuData->system, &cpuData->idle,
        &cpuData->iowait, &cpuData->irq, &cpuData->softirq, &cpuData->steal);
    fclose(file);
}

// Function for the Reader thread
void* readerThread(void* arg) {
    while (threadsRunning) {
        // Read CPU data
        CPUData cpuData;
        readCPUData(&cpuData);

        // Update global CPU usage data
        pthread_mutex_lock(&cpuUsageMutex);
        for (int i = 0; i < NUM_CORES; i++) {
            cpuUsage[i].previous = cpuUsage[i].current;
            cpuUsage[i].current = cpuData;
        }
        pthread_mutex_unlock(&cpuUsageMutex);

        // Signal Analyzer thread
        pthread_cond_signal(&readerUpdated);

        usleep(READ_DELAY);  // Wait for 100 milliseconds
    }

    return NULL;
}

// Function for the Analyzer thread
void* analyzerThread(void* arg) {
    while (threadsRunning) {
        // Wait for CPU data update
        pthread_mutex_lock(&cpuUsageMutex);
        pthread_cond_wait(&readerUpdated, &cpuUsageMutex);

        // Calculate CPU usage
        for (int i = 0; i < NUM_CORES; i++) {
            unsigned long prevTotal = cpuUsage[i].previous.user + cpuUsage[i].previous.nice +
                cpuUsage[i].previous.system + cpuUsage[i].previous.idle +
                cpuUsage[i].previous.iowait + cpuUsage[i].previous.irq +
                cpuUsage[i].previous.softirq + cpuUsage[i].previous.steal;

            unsigned long currentTotal = cpuUsage[i].current.user + cpuUsage[i].current.nice +
                cpuUsage[i].current.system + cpuUsage[i].current.idle +
                cpuUsage[i].current.iowait + cpuUsage[i].current.irq +
                cpuUsage[i].current.softirq + cpuUsage[i].current.steal;

            unsigned long total = currentTotal - prevTotal;
            unsigned long idle = cpuUsage[i].current.idle - cpuUsage[i].previous.idle;

            cpuUsage[i].usage = (double)(total - idle) / (double)total * 100.0;
            //printf("Core: %d | %f  ",i, cpuUsage[i].usage);
        }
        pthread_mutex_unlock(&cpuUsageMutex);

        // Signal Printer thread
        pthread_cond_signal(&analyzerUpdate);
    }

    return NULL;
}

// Function for the Printer thread
void* printerThread(void* arg) {
    while (threadsRunning) {
        // Wait for CPU usage update
        pthread_mutex_lock(&cpuUsageMutex);
        pthread_cond_wait(&analyzerUpdate, &cpuUsageMutex);

        // Print average CPU usage
        double averageUsage = 0.0;
        for (int i = 0; i < NUM_CORES; i++) {
            averageUsage += cpuUsage[i].usage;
        }
        averageUsage /= NUM_CORES;

        printf("Average CPU usage: %.2f%%\n", averageUsage);

        pthread_mutex_unlock(&cpuUsageMutex);
    }

    return NULL;
}



int main() {
    pthread_t threads[NUM_THREADS];

    pthread_create(&threads[0], NULL, readerThread, NULL);
    pthread_create(&threads[1], NULL, analyzerThread, NULL);
    pthread_create(&threads[2], NULL, printerThread, NULL);

    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    pthread_mutex_destroy(&cpuUsageMutex);
    pthread_cond_destroy(&readerUpdated);
    pthread_cond_destroy(&analyzerUpdate);
    return 0;
}