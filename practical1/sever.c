#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BUFFER_SIZE 1024

void write_file(int sockfd) {
    int n;
    FILE *fp;
    char *filename = "received_file.txt";
    char buffer[BUFFER_SIZE];
    long file_size;

    // 1. Receive File Size
    if (recv(sockfd, &file_size, sizeof(file_size), 0) <= 0) {
        perror("Error receiving file size");
        return;
    }
    printf("[-] Expecting file size: %ld bytes\n", file_size);

    fp = fopen(filename, "wb");
    if (fp == NULL) {
        perror("[-] Error creating file");
        return;
    }

    // 2. Loop to receive data until file size is reached
    long total_received = 0;
    while (total_received < file_size) {
        n = recv(sockfd, buffer, BUFFER_SIZE, 0);
        if (n <= 0) {
            break;
        }
        fwrite(buffer, 1, n, fp);
        total_received += n;
        bzero(buffer, BUFFER_SIZE);
    }
    
    printf("[+] File received successfully. Saved as '%s'\n", filename);
    fclose(fp);
}

int main() {
    int server_fd, new_sock;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    // 1. Create Socket
    // AF_INET = IPv4, SOCK_STREAM = TCP
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("[-] Socket failed");
        exit(EXIT_FAILURE);
    }

    // 2. Bind parameters
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; // Listen on any IP
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("[-] Bind failed");
        exit(EXIT_FAILURE);
    }
    printf("[+] Server listening on port %d...\n", PORT);

    // 3. Listen
    if (listen(server_fd, 3) < 0) {
        perror("[-] Listen failed");
        exit(EXIT_FAILURE);
    }

    // 4. Accept Connection
    if ((new_sock = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("[-] Accept failed");
        exit(EXIT_FAILURE);
    }
    printf("[+] Connection accepted from client.\n");

    // 5. Handle File Transfer
    write_file(new_sock);

    close(new_sock);
    close(server_fd);
    return 0;
}