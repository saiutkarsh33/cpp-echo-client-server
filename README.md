# TCP & UDP Echo Servers

This repository contains simple echo server implementations for both TCP and UDP, along with variations that demonstrate:

- **Multi‐threaded TCP echo server**: handling each incoming connection in its own thread.  
- **Non‐blocking I/O + multiplexing** using both `epoll()` (Linux) and `kqueue()` (BSD/macOS).
