#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>

#define MAX_WORDS 10000
#define MAX_WORD_LEN 50

// Structure to represent a key-value pair
typedef struct {
    char word[MAX_WORD_LEN];
    int count;
} KeyValue;

// Thread argument structure
typedef struct {
    char *text_chunk;
    KeyValue *results;
    int result_count;
} ThreadArgs;

// --- MAPPER FUNCTION ---
// Parses text and emits (word, 1) pairs into a local array
void *mapper(void *arg) {
    ThreadArgs *args = (ThreadArgs *)arg;
    args->result_count = 0;
    
    char *ptr = args->text_chunk;
    char word[MAX_WORD_LEN];
    int w_idx = 0;

    // Simple parsing loop
    for (int i = 0; ptr[i] != '\0'; i++) {
        if (isalpha(ptr[i])) {
            if (w_idx < MAX_WORD_LEN - 1) word[w_idx++] = tolower(ptr[i]);
        } else {
            if (w_idx > 0) {
                word[w_idx] = '\0';
                // Emit (Word, 1)
                strcpy(args->results[args->result_count].word, word);
                args->results[args->result_count].count = 1;
                args->result_count++;
                w_idx = 0;
                
                if (args->result_count >= MAX_WORDS) break; // Safety break
            }
        }
    }
    return NULL;
}

// Comparison function for qsort (The "Shuffle" logic)
int compare_kv(const void *a, const void *b) {
    return strcmp(((KeyValue *)a)->word, ((KeyValue *)b)->word);
}

// --- REDUCER FUNCTION ---
// Iterates over sorted pairs and aggregates counts
void reducer(KeyValue *all_data, int total_count) {
    if (total_count == 0) return;

    printf("----- Final Word Counts -----\n");
    
    char current_word[MAX_WORD_LEN];
    strcpy(current_word, all_data[0].word);
    int current_sum = 0;

    for (int i = 0; i < total_count; i++) {
        if (strcmp(all_data[i].word, current_word) == 0) {
            current_sum += all_data[i].count;
        } else {
            // New word encountered, emit previous result
            printf("%s: %d\n", current_word, current_sum);
            strcpy(current_word, all_data[i].word);
            current_sum = all_data[i].count;
        }
    }
    // Emit last word
    printf("%s: %d\n", current_word, current_sum);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // 1. Read File into Memory
    FILE *fp = fopen(argv[1], "r");
    if (!fp) { perror("File open error"); return 1; }
    
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    rewind(fp);
    
    char *buffer = malloc(fsize + 1);
    fread(buffer, 1, fsize, fp);
    buffer[fsize] = '\0';
    fclose(fp);

    // 2. Split Data (Input Splitting)
    // We split the buffer roughly in half for 2 threads
    long mid = fsize / 2;
    while (mid < fsize && isalpha(buffer[mid])) mid++; // Move to end of word
    
    char *chunk1 = buffer;
    char *chunk2 = buffer + mid + 1;
    buffer[mid] = '\0'; // Null-terminate first chunk

    // 3. Prepare Mapper Threads
    pthread_t t1, t2;
    ThreadArgs args1, args2;

    args1.text_chunk = chunk1;
    args1.results = malloc(MAX_WORDS * sizeof(KeyValue));
    
    args2.text_chunk = chunk2;
    args2.results = malloc(MAX_WORDS * sizeof(KeyValue));

    printf("[Master] Starting 2 Mapper threads (pthreads)...\n");
    pthread_create(&t1, NULL, mapper, &args1);
    pthread_create(&t2, NULL, mapper, &args2);

    pthread_join(t1, NULL);
    pthread_join(t2, NULL);

    // 4. Shuffle & Sort Phase
    // Combine all results into one array
    int total_items = args1.result_count + args2.result_count;
    KeyValue *all_data = malloc(total_items * sizeof(KeyValue));
    
    memcpy(all_data, args1.results, args1.result_count * sizeof(KeyValue));
    memcpy(all_data + args1.result_count, args2.results, args2.result_count * sizeof(KeyValue));

    // Sort to group identical words together
    qsort(all_data, total_items, sizeof(KeyValue), compare_kv);

    // 5. Run Reducer
    reducer(all_data, total_items);

    // Cleanup
    free(buffer);
    free(args1.results);
    free(args2.results);
    free(all_data);

    return 0;
}