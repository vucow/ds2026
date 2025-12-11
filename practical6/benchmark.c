#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>

#define SMALL_FILE_COUNT 1000
#define SMALL_FILE_SIZE 1024       // 1KB
#define LARGE_FILE_SIZE 104857600  // 100MB
#define BUFFER_SIZE 4096

// Timer helper
double get_time_diff(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1e9;
}

// 1. Small Files Benchmark
void benchmark_small_files(const char *dir) {
    printf("[*] Starting Small File Benchmark (Creating %d files)...\n", SMALL_FILE_COUNT);
    char filepath[256];
    char buffer[SMALL_FILE_SIZE];
    memset(buffer, 'A', SMALL_FILE_SIZE);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < SMALL_FILE_COUNT; i++) {
        snprintf(filepath, sizeof(filepath), "%s/small_%d.dat", dir, i);
        FILE *fp = fopen(filepath, "wb");
        if (fp) {
            fwrite(buffer, 1, SMALL_FILE_SIZE, fp);
            fclose(fp);
        } else {
            perror("File error");
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = get_time_diff(start, end);
    printf("--> Result: %.2f accesses/sec (Total time: %.2fs)\n", SMALL_FILE_COUNT / elapsed, elapsed);
    
    // Cleanup
    for (int i = 0; i < SMALL_FILE_COUNT; i++) {
        snprintf(filepath, sizeof(filepath), "%s/small_%d.dat", dir, i);
        unlink(filepath);
    }
}

// 2. Large File Benchmark (Write & Read)
void benchmark_large_file(const char *dir) {
    printf("\n[*] Starting Large File Benchmark (100MB)...\n");
    char filepath[256];
    snprintf(filepath, sizeof(filepath), "%s/large_test.dat", dir);
    
    char *buffer = malloc(BUFFER_SIZE);
    memset(buffer, 'B', BUFFER_SIZE);

    // Write Phase
    FILE *fp = fopen(filepath, "wb");
    if (!fp) { perror("Write open error"); return; }
    
    for (int i = 0; i < (LARGE_FILE_SIZE / BUFFER_SIZE); i++) {
        fwrite(buffer, 1, BUFFER_SIZE, fp);
    }
    fclose(fp);

    // Read Phase
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    fp = fopen(filepath, "rb");
    if (!fp) { perror("Read open error"); return; }
    
    while (fread(buffer, 1, BUFFER_SIZE, fp) > 0) {} // Consume file
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    fclose(fp);

    double elapsed = get_time_diff(start, end);
    double mb = (double)LARGE_FILE_SIZE / (1024 * 1024);
    printf("--> Result: %.2f MB/s (Time: %.2fs)\n", mb / elapsed, elapsed);

    // Cleanup
    unlink(filepath);
    free(buffer);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <mount_point>\n", argv[0]);
        return 1;
    }

    printf("Benchmarking GlusterFS at: %s\n", argv[1]);
    benchmark_small_files(argv[1]);
    benchmark_large_file(argv[1]);

    return 0;
}