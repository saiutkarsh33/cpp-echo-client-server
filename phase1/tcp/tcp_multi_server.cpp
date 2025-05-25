#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>

void handle_client(int client_fd, sockaddr_in cli) {
    char buf[1024];
    std::cout << "Child process handling client "
              << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port) << "\n";

    while (true) {
        ssize_t n = recv(client_fd, buf, sizeof(buf) - 1, 0);
        if (n <= 0) {
            if (n < 0) perror("recv");
            break; // client disconnected or error
        }

        buf[n] = '\0';
        std::cout << "Received from " << inet_ntoa(cli.sin_addr) << ": " << buf << "\n";

        if (send(client_fd, buf, n, 0) < 0) {
            perror("send");
            break;
        }
    }

    std::cout << "Client disconnected.\n";
    close(client_fd);
    exit(0); // only child exits
}



