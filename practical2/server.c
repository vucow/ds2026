#include <stdio.h>
#include "transfer.h" // Generated automatically by rpcgen

/* * This is the server function implementation.
 * It is called automatically by the skeleton when a request arrives.
 */
int *
upload_file_1_svc(file_data *argp, struct svc_req *rqstp)
{
    static int  result;
    FILE *fp;
    char output_path[300];

    printf("[Server] Receiving file request: %s (%d bytes)\n", argp->filename, argp->data_len);

    // Save with a prefix to avoid overwriting the original if testing locally
    snprintf(output_path, sizeof(output_path), "received_%s", argp->filename);

    fp = fopen(output_path, "wb");
    if (fp == NULL) {
        perror("[Server] File open error");
        result = 0; // Return 0 for failure
        return &result;
    }

    // Write the raw bytes (opaque data) to the file
    fwrite(argp->content.content_val, 1, argp->content.content_len, fp);
    fclose(fp);

    printf("[Server] File saved as '%s'\n", output_path);
    result = 1; // Return 1 for success

    return &result;
}