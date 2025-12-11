#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TAG_FILENAME 0
#define TAG_FILESIZE 1
#define TAG_FILEDATA 2
#define SENDER_RANK 1
#define RECEIVER_RANK 0

// --- SENDER (CLIENT) LOGIC ---
void run_client(char *filename) {
    FILE *fp;
    long file_size;
    char *buffer;

    // 1. Open the file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        fprintf(stderr, "[Rank 1] Error: Could not open file '%s'\n", filename);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // 2. Determine file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    printf("[Rank 1] Sending file '%s' (%ld bytes) to Rank 0...\n", filename, file_size);

    // 3. Send Filename (so receiver knows what to save it as)
    // We send the length + 1 to include the null terminator
    MPI_Send(filename, strlen(filename) + 1, MPI_CHAR, RECEIVER_RANK, TAG_FILENAME, MPI_COMM_WORLD);

    // 4. Send File Size
    MPI_Send(&file_size, 1, MPI_LONG, RECEIVER_RANK, TAG_FILESIZE, MPI_COMM_WORLD);

    // 5. Read and Send File Content
    buffer = (char *)malloc(file_size);
    fread(buffer, 1, file_size, fp);
    
    MPI_Send(buffer, file_size, MPI_CHAR, RECEIVER_RANK, TAG_FILEDATA, MPI_COMM_WORLD);

    printf("[Rank 1] Transfer complete.\n");

    free(buffer);
    fclose(fp);
}

// --- RECEIVER (SERVER) LOGIC ---
void run_server() {
    MPI_Status status;
    char filename[256];
    long file_size;
    char *buffer;
    FILE *fp;

    printf("[Rank 0] Waiting for incoming connection...\n");

    // 1. Receive Filename
    // Probe or just receive with a large enough buffer
    MPI_Recv(filename, 256, MPI_CHAR, SENDER_RANK, TAG_FILENAME, MPI_COMM_WORLD, &status);

    // 2. Receive File Size
    MPI_Recv(&file_size, 1, MPI_LONG, SENDER_RANK, TAG_FILESIZE, MPI_COMM_WORLD, &status);

    printf("[Rank 0] Receiving '%s' (%ld bytes)...\n", filename, file_size);

    // 3. Allocate Memory and Receive Data
    buffer = (char *)malloc(file_size);
    MPI_Recv(buffer, file_size, MPI_CHAR, SENDER_RANK, TAG_FILEDATA, MPI_COMM_WORLD, &status);

    // 4. Write to disk
    // Prepend "received_" to verify it's a new file
    char output_name[300];
    snprintf(output_name, sizeof(output_name), "received_%s", filename);

    fp = fopen(output_name, "wb");
    if (fp) {
        fwrite(buffer, 1, file_size, fp);
        fclose(fp);
        printf("[Rank 0] File saved successfully as '%s'.\n", output_name);
    } else {
        printf("[Rank 0] Error opening file for writing.\n");
    }

    free(buffer);
}

int main(int argc, char *argv[]) {
    int rank, size;

    // Initialize MPI Environment
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank); // Who am I?
    MPI_Comm_size(MPI_COMM_WORLD, &size); // How many processes?

    if (size < 2) {
        if (rank == 0) {
            fprintf(stderr, "Error: This program requires at least 2 processes (Sender and Receiver).\n");
            fprintf(stderr, "Usage: mpiexec -n 2 ./mpi_transfer filename\n");
        }
        MPI_Finalize();
        return 1;
    }

    if (rank == SENDER_RANK) {
        if (argc < 2) {
            fprintf(stderr, "[Rank 1] Error: Please provide a filename.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        run_client(argv[1]);
    } else if (rank == RECEIVER_RANK) {
        run_server();
    }

    MPI_Finalize();
    return 0;
}