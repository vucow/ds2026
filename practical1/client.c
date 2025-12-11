#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SERVER_IP "127.0.0.1" // Change this if running on different machines
#define BUFFER_SIZE 1024

void send_file(FILE *fp, int sockfd) {
    char data[BUFFER_SIZE] = {0};
    long file_size;

    // Get file size
    fseek(fp, 0, SEEK_END);
    file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET); // Reset pointer to start

    // 1. Send File Size
    if (send(sockfd, &file_size, sizeof(file_size), 0) == -1) {
        perror("[-] Error sending file size");
        exit(1);
    }
    printf("[+] File size sent: %ld bytes\n", file_size);

    // 2. Send File Content
    while (fgets(data, BUFFER_SIZE, fp) != NULL) {
        if (send(sockfd, data, sizeof(data), 0) == -1) {
            perror("[-] Error in sending file.");
            exit(1);
        }
        bzero(data, BUFFER_SIZE);
    }
    printf("[+] File data sent successfully.\n");
}

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char *filename = "send_me.txt"; // Ensure this file exists!

    // 1. Create Socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\n[-] Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        printf("\n[-] Invalid address/ Address not supported \n");
        return -1;
    }

    // 2. Connect
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\n[-] Connection Failed \n");
        return -1;
    }
    printf("[+] Connected to server.\n");

    // 3. Open File
    FILE *fp = fopen(filename, "rb");
    if (fp == NULL) {
        perror("[-] Error reading file");
        return 1;
    }

    // 4. Send File
    send_file(fp, sock);
    
    fclose(fp);
    close(sock);
    return 0;
}