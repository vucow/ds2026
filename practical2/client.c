#include <stdio.h>
#include <stdlib.h>
#include "transfer.h" // Generated automatically by rpcgen

void
file_transfer_prog_1(char *host, char *filename)
{
    CLIENT *clnt;
    int  *result_1;
    file_data  upload_file_1_arg;
    FILE *fp;
    long file_size;
    char *buffer;

    // 1. Create RPC Client Handle
    clnt = clnt_create(host, FILE_TRANSFER_PROG, FILE_TRANSFER_VERS, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }

    // 2. Read the local file
    fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("Error opening local file");
        exit(1);
    }

    // Determine file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    rewind(fp);

    // Allocate memory and read file data
    buffer = (char *)malloc(file_size);
    if (buffer == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    fread(buffer, 1, file_size, fp);
    fclose(fp);

    // 3. Fill the RPC struct
    upload_file_1_arg.filename = filename;
    upload_file_1_arg.data_len = file_size;
    
    // For 'opaque' types, we must set .content_val (data) and .content_len (size)
    upload_file_1_arg.content.content_val = buffer;
    upload_file_1_arg.content.content_len = file_size;

    // 4. Call the Remote Procedure
    printf("[Client] Uploading '%s' (%ld bytes) to %s...\n", filename, file_size, host);
    
    result_1 = upload_file_1(&upload_file_1_arg, clnt);

    if (result_1 == (int *)NULL) {
        clnt_perror(clnt, "RPC Call Failed");
    } else {
        if (*result_1 == 1) {
            printf("[Client] Success: File transfer completed.\n");
        } else {
            printf("[Client] Error: Server failed to save file.\n");
        }
    }

    // Cleanup
    free(buffer);
    clnt_destroy(clnt);
}

int
main(int argc, char *argv[])
{
    char *host;
    char *filename;

    if (argc < 3) {
        printf("usage: %s server_host filename\n", argv[0]);
        exit(1);
    }
    host = argv[1];
    filename = argv[2];
    
    file_transfer_prog_1(host, filename);
    exit(0);
}