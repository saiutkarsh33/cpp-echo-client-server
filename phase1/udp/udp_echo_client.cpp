#include <iostream>        // std::cout, std::cin, std::getline
#include <cstring>         // std::memset(), std::strlen()
#include <sys/socket.h>    // socket(), sendto(), recvfrom()
#include <arpa/inet.h>     // sockaddr_in, inet_pton(), htons(), ntohs()
#include <unistd.h>        // close()


int main() {
    // 1) Create the UDP socket (client) 
    // 0 means default datagram protocol, UDP
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return 1;
    }

    // Set up server address

    sockaddr_in serv{};
    std::memset(&serv, 0, sizeof(serv));      // zero out the struct
    serv.sin_family = AF_INET;                 // IPv4
    serv.sin_port   = htons(8080);             // port 8080 in network byte order

    //inet_pton parses the string '127.0.0.1' into 4 8bit numbers - 127,0,0,1 and packs them into a single 32 bit value in network byte order
    // and writes that into serv.sin_addr
    // u pas in the address family AF_INET so the fn knows to parse a dotted decimal IPv4 string.
    inet_pton(AF_INET, "127.0.0.1", &serv.sin_addr); 

    
    // size of the server address, needed by sendto()
    socklen_t serv_len = sizeof(serv);    

    // 3) send → receive loop
    while (true) {
        char buf[1024];
        // a) read a line from stdin
        std::cout << "> ";
        std::string line;
        if (!std::getline(std::cin, line))
            break;                      // EOF (ctrl D) or error → exit loop
        
        // b) send the line as a UDP datagram
        // 0 means no special flags
        // need to pass in serv len because u give a generic sockaddr*, so kernel needs to know how many bytes shld look at to read the full address of server 
        // In modern C++, reinterpret_cast is safer because the compiler checks whether the conversion is allowed and doesent silently convert things that are windly unsafe 
        if (sendto(sockfd,
                   line.c_str(), line.size(),
                   0,
                   reinterpret_cast<sockaddr*>(&serv),
                   serv_len) < 0)
        {
            perror("sendto");
            continue;                  // try again on next iteration
        }
        // c) wait for the echo reply
        ssize_t n = recvfrom(sockfd,
                             buf, sizeof(buf)-1,
                             0,
                             nullptr, nullptr);
        if (n < 0) {
            perror("recvfrom");
            continue;
        }
        buf[n] = '\0';                  // NUL-terminate
        std::cout << "echoed: " << buf << "\n";

    }
    // after user pressed ctrl D
    // ctrl D sends an EOF marker, doesent kill the process, just tells the std::getline() that theres no more input
    close(sockfd);
    return 0;
}

