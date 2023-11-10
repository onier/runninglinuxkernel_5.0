//gcc -o test_mychardev test_mychardev.c -lpthread
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_THREADS 3
#define FILE_PATH "/dev/mychardev"

// Write function for threads
void *writeThread(void *arg) {
    FILE *file = fopen(FILE_PATH, "w");
    if (file == NULL) {
        perror("Error opening file");
        pthread_exit(NULL);
    }

    fprintf(file, "Hello from thread %d:!\n", (long) arg);

    fclose(file);
    pthread_exit(NULL);
}

// Write function for processes
void writeProcess(int i) {
    FILE *file = fopen(FILE_PATH, "w");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "Hello from process %d !\n", i);

    fclose(file);
    exit(EXIT_SUCCESS);
}

int main() {
    // Create threads
    pthread_t threads[NUM_THREADS];
    for (long i = 0; i < NUM_THREADS; ++i) {
        if (pthread_create(&threads[i], NULL, writeThread, (void *) i) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    // Create processes
    for (int i = 0; i < NUM_THREADS; ++i) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Error creating process");
            exit(EXIT_FAILURE);
        } else if (pid == 0) { // Child process
            writeProcess(i);
        }
    }

    // Wait for threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Error joining thread");
            exit(EXIT_FAILURE);
        }
    }

    // Wait for processes to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        int status;
        if (wait(&status) == -1) {
            perror("Error waiting for process");
            exit(EXIT_FAILURE);
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Child process exited with non-zero status\n");
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}
