#include <iostream>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <arpa/inet.h>

int make_non_blocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int main() {
    // 1. Create and bind TCP socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    

    sockaddr_in serv{};
    serv.sin_family = AF_INET;
    serv.sin_addr.s_addr = INADDR_ANY;
    serv.sin_port = htons(8080);

    // Server must bind to a port so clients know where to fid it
    // Without bind(), theres no fixed address the OS would associate with my listening socket
    

    // Why doesent the client need to bind()?
    // OS can choose any avail local port for the client automatically
    // Clients only care where theyre gg not where theyre calling from
    // connect() on the client implicitly binds to an ephermal port like 40023 and picks the IP if not already bound
    // client can bind() when you wanna specify the local IP or port due to legacy reasons / firewall rules
    // or when you want like a bi directional communication where both kinda act like a server (peer to peer) [IN UDP]

    if (bind(server_fd, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("bind");
        return 1;
    }
    //  transitions a socket from: "inactive" ➝ "listening for connections
    if (listen(server_fd, SOMAXCONN) < 0) {
        perror("listen");
        return 1;
    }

    make_non_blocking(server_fd);


    // 2. Set up kqueue
    int kq = kqueue();
    if (kq < 0) { perror("kqueue"); return 1; }
    
    // ev_set is for configuration
    struct kevent ev_set;
    // fills the struct w server_fd (fd u wanna monitor like a listening socket),
    // EVFILT_READ tells to kernel to notify when theres something to read
    // EV_ADD  — registers this FD into the kqueue (kernels internal red black tree of monitored events)
    // EV_ENABLE enables delivery of events 
    EV_SET(&ev_set, server_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
    // submits the event to the kernel, hey kernel, monitor this FD for readability
    kevent(kq, &ev_set, 1, nullptr, 0, nullptr);    

    // Above you're monitoring server_fd, the listening socket.
    // You're telling the kernel: "Notify me when a new client tries to connect."
    // accept() won't block — instead, your server process sleeps inside kevent().
    // This is more efficient than blocking I/O, where the process actively waits (blocking and holding up CPU).
    // With kqueue, the kernel wakes you only when needed, avoiding CPU waste.
    // In general. accept() is a blocking call
    
    std::cout << "TCP server listening on port 8080 (kqueue)...\n";

    // A buffer where kevent() will write the list of FDs that are ready for i/o
    std::vector<struct kevent> ev_list(32);
    char buf[1024];

    while (true) {
        // Put thread to sleep until kernel says that an fd is ready to be read
        // This fd cld be server fd (means theres a new client connection)
        // or client fd (the client needed has data ready)
        // Add the fd to the ev list, up to 32
        int nev = kevent(kq, nullptr, 0, ev_list.data(), ev_list.size(), nullptr);

        // ev list is a buffer that gets overwritten across every kevent call



        for (int i = 0; i < nev; ++i) {
            int fd = ev_list[i].ident;
            if (fd == server_fd) {
                // New client connection
                sockaddr_in cli{};
                socklen_t len = sizeof(cli);
                // accept the client - server connection
                int client_fd = accept(server_fd, (sockaddr*)&cli, &len);
                if (client_fd >= 0) {
                    // make the fd non blocking
                    make_non_blocking(client_fd);
                    // Hey kernel, please start watching this new client socket 
                    // and notify me when data has been read

                    //ev_set is not a permanent list of all events.

                    //It’s a command struct that you reuse each time you want to tell the kernel to add, delete, or modify an event.
                    EV_SET(&ev_set, client_fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, nullptr);
                    
                    // Registers the above event into the kernels internal kqueue event list (rb tree)
                    kevent(kq, &ev_set, 1, nullptr, 0, nullptr);
                    std::cout << "Accepted connection from "
                              << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port) << "\n";
                } 
            } else if (ev_list[i].filter == EVFILT_READ) {
                // Means that there is a client ready to be read
                ssize_t n = read(fd, buf, sizeof(buf) - 1);
                // n = 0 means client closed connection
                // -1 means error, cld be disconnection too?
                if (n <= 0) {
                    std::cout << "Client disconnected (fd " << fd << ")\n";
                    close(fd);
                    EV_SET(&ev_set, fd, EVFILT_READ, EV_DELETE, 0, 0, nullptr);
                    kevent(kq, &ev_set, 1, nullptr, 0, nullptr);
                } else {
                    buf[n] = '\0';
                    std::cout << "From fd " << fd << ": " << buf << "\n";
                    write(fd, buf, n);  // Echo back
                    // dont remove from ev event bc its a buffer that is reinitalised on top of every call
                    // you dont remove fd from kqueue bc u still care if its giving new data (client connection alr made)
                }
            }
        }
    }
    close(server_fd);
    close(kq);
    return 0;    

}