#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int main() {
    // 1. Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // 2. Set up the server address
    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_port = htons(8080);
    if (inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr) <= 0) {
        perror("inet_pton");
        return 1;
    }

    // 3. Connect to the server
// When you call connect(), the kernel auto-fills the missing info:

// Chooses a local ephemeral port (like 48912) from a valid dynamic port range (e.g., 49152–65535)

// Assigns a source IP address based on the routing table (i.e., the interface used to reach the server)

// Internally performs the 3-way TCP handshake
    if (connect(sockfd, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("connect");
        return 1;
    }

    std::cout << "Connected to TCP echo server at 127.0.0.1:8080\n";

    char buf[1024];
    std::string line;    

    // 4. Send-receive loop
    while (true) {
        std::cout << "> ";
        if (!std::getline(std::cin, line)) break;

        if (send(sockfd, line.c_str(), line.size(), 0) < 0) {
            perror("send");
            break;
        }

        ssize_t n = recv(sockfd, buf, sizeof(buf)-1, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            break;
        }

        buf[n] = '\0';
        std::cout << "echoed: " << buf << "\n";
    }
    close(sockfd);
    return 0;

}