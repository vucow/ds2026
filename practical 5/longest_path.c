#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_PATH_LEN 4096

// Structure to hold the result from a Mapper
typedef struct {
    char longest_path[MAX_PATH_LEN];
    int length;
} MapperResult;

// Structure to pass arguments to the Mapper thread
typedef struct {
    char *filename;
    MapperResult *result; // Pointer to where the mapper should write its result
} MapperArgs;

// --- MAPPER FUNCTION ---
// Reads one file line-by-line and finds the longest path in it.
// This effectively reduces the data volume significantly before the "Shuffle" phase.
void *mapper(void *arg) {
    MapperArgs *args = (MapperArgs *)arg;
    FILE *fp = fopen(args->filename, "r");
    
    // Initialize result
    args->result->length = 0;
    args->result->longest_path[0] = '\0';

    if (fp == NULL) {
        fprintf(stderr, "[Mapper] Error: Could not open file %s\n", args->filename);
        return NULL;
    }

    char line[MAX_PATH_LEN];
    while (fgets(line, sizeof(line), fp)) {
        // Strip newline characters
        line[strcspn(line, "\r\n")] = 0;
        
        int len = strlen(line);
        if (len > args->result->length) {
            args->result->length = len;
            strcpy(args->result->longest_path, line);
        }
    }
    
    fclose(fp);
    printf("[Mapper] Finished %s. Longest here: %d chars.\n", args->filename, args->result->length);
    return NULL;
}

// --- REDUCER FUNCTION ---
// Takes the best candidate from every mapper and finds the absolute winner.
void reducer(MapperResult *results, int count) {
    printf("\n[Reducer] Aggregating results from %d mappers...\n", count);
    
    int max_len = -1;
    char global_longest[MAX_PATH_LEN] = "";
    int winner_idx = -1;

    for (int i = 0; i < count; i++) {
        if (results[i].length > max_len) {
            max_len = results[i].length;
            strcpy(global_longest, results[i].longest_path);
            winner_idx = i;
        }
    }

    if (winner_idx != -1) {
        printf("--------------------------------------------------\n");
        printf("FINAL RESULT: The Longest Path\n");
        printf("--------------------------------------------------\n");
        printf("Length: %d characters\n", max_len);
        printf("Path:   %s\n", global_longest);
    } else {
        printf("No paths found.\n");
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <file1> <file2> ... <fileN>\n", argv[0]);
        printf("Example: %s laptop1.log laptop2.log\n", argv[0]);
        return 1;
    }

    int num_files = argc - 1;
    pthread_t threads[num_files];
    MapperArgs thread_args[num_files];
    MapperResult results[num_files];

    printf("[Master] Spawning %d Mapper threads...\n", num_files);

    // 1. Map Phase (Parallel)
    for (int i = 0; i < num_files; i++) {
        thread_args[i].filename = argv[i + 1];
        thread_args[i].result = &results[i]; // Mapper writes directly to this slot
        
        if (pthread_create(&threads[i], NULL, mapper, &thread_args[i]) != 0) {
            perror("Failed to create thread");
            return 1;
        }
    }

    // 2. Wait for Mappers
    for (int i = 0; i < num_files; i++) {
        pthread_join(threads[i], NULL);
    }

    // 3. Reduce Phase
    reducer(results, num_files);

    return 0;
}