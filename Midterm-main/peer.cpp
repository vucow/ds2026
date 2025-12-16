#include <arpa/inet.h>
#include <cstring>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

using namespace std;

void receive_file(int port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    bind(server_fd, (sockaddr*)&address, sizeof(address));
    listen(server_fd, 1);

    cout << "[Receiver] Waiting for connection...\n";
    int client_fd = accept(server_fd, nullptr, nullptr);
    cout << "[Receiver] Client connected\n";

    uint32_t name_len;
    recv(client_fd, &name_len, sizeof(name_len), MSG_WAITALL);

    string filename(name_len, '\0');
    recv(client_fd, filename.data(), name_len, MSG_WAITALL);

    uint64_t file_size;
    recv(client_fd, &file_size, sizeof(file_size), MSG_WAITALL);

    cout << "[Receiver] Receiving file: " << filename
         << " (" << file_size << " bytes)\n";

    ofstream outfile(filename, ios::binary);

    char buffer[4096];
    uint64_t received = 0;

    while (received < file_size) {
        ssize_t bytes = recv(client_fd, buffer,
                              min(sizeof(buffer), file_size - received), 0);
        if (bytes <= 0) break;
        outfile.write(buffer, bytes);
        received += bytes;
    }

    cout << "[Receiver] File received successfully\n";

    outfile.close();
    close(client_fd);
    close(server_fd);
}

void send_file(const char* ip, int port, const char* filename) {
    int sock;
    struct sockaddr_in serv_addr{};

    ifstream infile(filename, ios::binary | ios::ate);
    if (!infile.is_open()) {
        cerr << "Failed to open input file\n";
        return;
    }

    uint64_t file_size = infile.tellg();
    infile.seekg(0);

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket failed");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_pton(AF_INET, ip, &serv_addr.sin_addr);

    connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
    cout << "[Sender] Connected\n";

    uint32_t name_len = strlen(filename);
    send(sock, &name_len, sizeof(name_len), 0);

    send(sock, filename, name_len, 0);

    send(sock, &file_size, sizeof(file_size), 0);
    
    char buffer[4096];
    while (infile.read(buffer, sizeof(buffer))) {
        send(sock, buffer, infile.gcount(), 0);
    }
    if (infile.gcount() > 0)
        send(sock, buffer, infile.gcount(), 0);

    cout << "[Sender] File sent successfully (" << file_size << " bytes)\n";

    infile.close();
    close(sock);
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        cout << "Usage:\n"
             << "  Receive: ./peer receive <port>\n"
             << "  Send:    ./peer send <ip> <port> <filename>\n";
        return 1;
    }

    string mode = argv[1];

    if (mode == "receive") {
        int port = stoi(argv[2]);
        receive_file(port);
    } else if (mode == "send") {
        if (argc < 5) {
            cout << "Usage: ./peer send <ip> <port> <filename>\n";
            return 1;
        }
        send_file(argv[2], stoi(argv[3]), argv[4]);
    } else {
        cout << "Invalid mode. Use 'send' or 'receive'.\n";
    }

    return 0;
}