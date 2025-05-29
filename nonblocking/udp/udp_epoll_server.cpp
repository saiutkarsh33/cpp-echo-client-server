#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <arpa/inet.h>

int make_non_blocking(int fd) {
    // Retrieves current flags on fd
    // Adds O_NONBLOCK so syscalls like recvfrom() return -1 
    // and set errno = EAGAIN if nothing is available
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    int udp_fd = socket(AF_INET, SOCK_DGRAM, 0);

    if (udp_fd < 0) { perror("socket"); return 1; }

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(8080);


    if (bind(udp_fd, (sockaddr*)&serv, sizeof(serv)) < 0) {
    perror("bind");
    return 1;
    }
    make_non_blocking(udp_fd);
    
// epfd is a file descriptor used to refer to this epoll set
// Epoll will monitor one or more FDs for I/O events    
    int epfd = epoll_create1(0);

    epoll_event ev{};
    // you want the event to be where theres
    // data available to read
    ev.events = EPOLLIN;
    // store servers fd in data.fd
    ev.data.fd = udp_fd;
    // add the fd to epolls RB tree
    epoll_ctl(epfd, EPOLL_CTL_ADD, udp_fd, &ev);    

    std::cout << "UDP server listening on port 8080 (epoll, non-blocking)...\n";
    
    epoll_event events[10];
    char buf[1024];   

    while (true) {
    // epoll wait checks the ready queue, returns number of triggered FDs up to 10
    // epoll_wait blocks indefinitely with timeout = -1 (-1 means block forever)
   // if return == -1, an error occurred (e.g. signal interrupted)
    int n = epoll_wait(epfd, events, 10, -1);

    for (int i = 0; i < n; ++i) {
        if (events[i].data.fd == udp_fd) {
            sockaddr_in cli{};
            socklen_t cli_len = sizeof(cli);
            // recvfrom is nonblocking so will return -1 if not ready yet but epoll_wait() alr told us its ready 
            ssize_t len = recvfrom(udp_fd, buf, sizeof(buf) - 1, 0, (sockaddr*)&cli, &cli_len);
            if (len > 0) {
                buf[len] = '\0';
                std::cout << "Received from " << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port) << " → " << buf << "\n";
                // echo back to client
                sendto(udp_fd, buf, len, 0, (sockaddr*)&cli, cli_len);
            }
        }
    }
    }
    close(udp_fd);
    close(epfd);
    
}
// When a client sends a datagram, the kernel:

// Places the incoming packet in the receive buffer of udp_fd

// Triggers the readable callback → adds udp_fd to epoll's ready list

// Your next epoll_wait() returns udp_fd as ready

// Then you call recvfrom() to get both:

// The message

// The client’s IP/port (from the packet metadata)

