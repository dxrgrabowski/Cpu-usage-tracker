#include <stdio.h>
#include <time.h>

#define EXECUTION_LENGTH 6000 

void performCalculations() {
    int i, j;
    double result = 0.0;

    // Wykonaj intensywne obliczenia
    for (i = 0; i < 1000000; i++) {
        for (j = 0; j < 10000; j++) {
            result += i * j;
        }
    }
}

int main() {
    printf("The program starts to load the CPU...\n");

    clock_t startTime = clock();
    clock_t elapsedTime;

    while (1) {
        performCalculations();
        elapsedTime = (clock() - startTime) * 1000 / CLOCKS_PER_SEC; 

        if (elapsedTime >= EXECUTION_LENGTH) {
            break; 
        }
    }

    printf("The program has finished loading the processor.\n");

    return 0;
}
