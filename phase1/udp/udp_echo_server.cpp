#include <iostream>
#include <cstring>        // memset()
#include <sys/socket.h>   // socket(), bind(), recvfrom(), sendto()
#include <netinet/in.h>   // sockaddr_in, htons(), INADDR_ANY
#include <unistd.h>       // close()
#include <cstdlib>        // exit(), EXIT_FAILURE
#include <arpa/inet.h>



int main() {

    //create the socket
    // Address family here is the internet, others include AF_INET6 for IPV6, or AF_UNIX for local IPC
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    
    // returns -1 on failure so
    if (sockfd < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // create a socketaddr struct for server 
    sockaddr_in serv{};

    // This line of code takes in the address of the socketaddr_in serv struct, writes the byte value 0 (0000 0000) into each of the byte for sizeof the serv bytes
    // every member of serv, including any padding, is set to all zeroes
    // this prevents garvage values from sneaking into network calls
    
    // for example, padding bytes need to be zero or kernel will reject the address struct as invalid during bind()
    std::memset(&serv, 0, sizeof(serv));

    serv.sin_family      = AF_INET;
    
    // one machine can have different network interfaces each w a diff IP address (eg loopback(local only), ethernet (wired LAN), wireless network (widi), docker bridge (virtual network for containers) or VPN (secure tunnel))
    serv.sin_addr.s_addr = INADDR_ANY; // bind to all network interfaces (0.0.0.0) -> accepts datagram from any IP address' port 8080
    
    // Store port 8080 in network byte order (big-endian),
    // so that no matter the host’s native endianness, the kernel
    // and any remote peer will see it correctly as port 8080.
    serv.sin_port = htons(8080); 
    
// bind() returns -1 on error (and sets errno).
// It associates our socket (sockfd) with the local address/port in serv.
// Common reasons bind() can fail (errno set accordingly):
//   • EACCES        – trying to bind a “privileged” port (<1024) without root.
//   • EADDRINUSE    – the (IP,port) tuple is already in use by another socket.
//   • EADDRNOTAVAIL – the specified IP address isn’t local to this host.
//   • EBADF         – sockfd isn’t a valid file descriptor.
//   • EINVAL        – socket is already bound, or address family mismatch.
// Under the hood, bind() does roughly this in the kernel:
//   1. Verifies sockfd is a socket of the right family.
//   2. Checks that the address/port are valid and not already taken.
//   3. Records the (address,port) in the socket’s kernel data so incoming
//      packets or connection requests get routed to this socket.
// We cast &serv to (struct sockaddr*) because bind()’s signature is
//   int bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
// and sockaddr is the generic base‐type for all address families.
    if (bind(sockfd, (sockaddr*)&serv, sizeof(serv)) < 0) {
        perror("bind");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    std::cout << "Server listening on port 8080\n";


    // 1023 bytes + NULL TERMINATOR
// 3. Isn’t char buf[1024] only for characters?
// In C/C++, char is just the smallest addressable byte type. A char[] is really a byte‐array, and sockets deal in raw bytes.
// Whether you send ASCII text, integers, images, or serialized objects, it all ends up as a stream of bytes. You can reinterpret those bytes however you like


// Why do we reserve one byte for a NUL terminator in buf?
// you’re treating buf as a C-string for printing. C-string functions (printf("%s"), std::cout << buf, strlen(), etc.) expect a trailing '\0' to know where the text ends. If you omitted that:

// Printing routines would keep reading past your data into whatever garbage follows in memory → undefined behavior (often junk output or a crash).

// Substring or length-based logic would break, since there’s no marker for the end of “the string.”

// If you were only sending/receiving pure binary data (e.g. packed integers), you wouldn’t need a terminator—you’d work off the returned byte-count (n) instead of relying on '\0'

    char buf[1024];
    sockaddr_in cli{};
// cli_len tells the kernel how big your address buffer (cli) is so it won’t overrun it when writing the peer’s socket address
// Before the call: you’re saying “I have room for sizeof(cli) bytes of address data—please don’t write more than that.”
// After the call: the kernel updates cli_len to the real number of bytes it wrote (for IPv4, that stays sizeof(sockaddr_in)).

    socklen_t cli_len = sizeof(cli);

    // Below, the sockfd represents SERVERS ENDPOINT (its port/address)
    // and the cli struct represents the remote endpoint


// 1. “Same socket for all peers” analogy
// UDP is like having one single mailbox (your socket FD) at your house.

// Socket FD = your mailbox number.

// recvfrom() is you opening your mailbox and pulling out whatever letters (datagrams) have arrived from any sender.

// sendto(..., cli, cli_len) is you putting a letter addressed to that sender into your mailbox for collection.

// You don’t get a new mailbox for each friend who writes you—there’s just one. You look at the “From:” on each letter, decide if you want to reply, then drop your reply letter back into your own mailbox for the post office to deliver.

// By contrast, TCP is like having a dedicated private line or slot in a multi-box cabinet for each friend. When Alice calls (or connects), you get a dedicated handset (a new FD) you use for all your back-and-forth. Bob gets another handset. You keep track of each handset separately

    while (true) {
        // recvfrom is a blocking call, blocks until server receives a datagram. it then copies the first 1023 (or less) bytes received into the buffer, and the
        // cli and cli_len as well
        // returns the number of bytes received
        ssize_t n = recvfrom(sockfd, buf, sizeof(buf)-1, 0, reinterpret_cast<sockaddr*>(&cli), &cli_len);
        if (n < 0) { perror("recvfrom"); continue; }
        buf[n] = '\0';
        std::cout << "From " 
                  << inet_ntoa(cli.sin_addr) << ":" << ntohs(cli.sin_port)
                  << " → " << buf << "\n";
        // inet_ntoa converts binary ip to human readable string
        if (sendto(sockfd, buf, n, 0, (sockaddr*)&cli, cli_len) < 0) {
            perror("sendto");
        }
    } 
    
// Whenever your process terminates—whether by returning from main(), calling exit(), or being killed by a signal like Ctrl-C—the kernel will automatically close all of its open file descriptors (including your UDP socket). So you won’t leave the socket “leaking” at the OS level once the process is gone.
    // this is quite unreachable
    close(sockfd);
    return 0;  
}     









    













