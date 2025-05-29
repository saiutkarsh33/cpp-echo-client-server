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

int main() {
// Ignore SIGCHLD to auto-clean zombie processes.
// Normally, the parent is expected to call wait() to clean up each child,
// but since we may fork many children (one per client),
// we don't want the parent to block waiting on each of them.
// By setting SIGCHLD to SIG_IGN, we tell the kernel that we don't care
// about child exit status, and it can automatically reap them.
// This prevents zombie processes from accumulating in the process table.
    signal(SIGCHLD, SIG_IGN);

    // Create a socket w default TCP (0)
    // FD is my local handle to the server's listening socket in the kernel
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0) { perror("socket"); return 1; }

// Allow the server to reuse the port immediately after shutdown,
// even if it's in TIME_WAIT state (i.e., recently closed).
// Without this, restarting the server quickly might fail with
// "bind: Address already in use".
// This tells the OS: “It's okay, I know what I’m doing—let me reuse the port.”

// TCP needs SO_REUSEADDR because kernel keeps the port in the TIME_WAIT state for about
// 60 seconds. this is part of TCP's design to handle delayed or duplicate packets gracefully
// During this time, the kernel wont let another socket bind to the same IP:port unless SO_REUSEADDR is set
// So if u wanna restart the server, dont have to wait 60 seconds

// For UDP, there is no connection state to begin with
// when u close a udp socket, there is no time_wait and the port is released immediately
// so bind() on restart just works off the gate, dont need to care bout setsockopt() etc
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(8080);
    

    // bind the port to local address:port

    if (bind(server_fd, (sockaddr*)&serv, sizeof(serv)) < 0) {
    perror("bind"); close(server_fd); return 1;
    }
    
    // 5 is backlog: queue up to 5 unaccepted incoming connections
    // before rejecting new ones
    if (listen(server_fd, 5) < 0) {
    perror("listen"); close(server_fd); return 1;
    }

    std::cout << "TCP multi-client echo server listening on port 8080...\n";
    

    while (true) {
        
        // Prepare a struct to receive the client's address when a new connection comes in.
        sockaddr_in cli{};
        socklen_t cli_len = sizeof(cli);
        
        // blocking call: waits for an incoming connection
        // cli is filled w client's IP + port
        // new fd is returned (just a way to enter kenrnel place)
        int client_fd = accept(server_fd, (sockaddr*)&cli, &cli_len);

        if (client_fd < 0) {
            perror("accept");
            continue;
        }
        
        // if pid == 0, we are child
        // if pid > 0, we r parent
        // if pid < 0, error
        pid_t pid = fork();

        if (pid == 0) {
        // Child process
        close(server_fd); // child doesn't need listener
        handle_client(client_fd, cli);
        }
        else if (pid > 0) {
        // Parent process
        close(client_fd); // parent doesn't use this client FD
        }
        else {
        perror("fork");
        close(client_fd);
        }
    }
    close(server_fd);
    return 0;
}


