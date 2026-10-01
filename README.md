# Layer 4 TCP Load Balancer

A lightweight, multithreaded Layer 4 TCP Load Balancer written purely in C. This project demonstrates core systems programming concepts, including network sockets, concurrency, and inter-process synchronization.

## Features

- **Layer 4 Proxying**: Forwards raw TCP traffic (HTTP, etc.) between clients and backend servers.
- **Round-Robin Scheduling**: Distributes incoming connections evenly across available backends.
- **Active Health Checks**: A dedicated background thread periodically checks the status of backend servers. Unhealthy backends are automatically removed from the rotation until they recover.
- **Concurrency**: Uses POSIX Threads (`pthreads`) to handle multiple client connections simultaneously without blocking the main server thread.
- **Thread Safety**: Uses mutexes to prevent race conditions when updating backend availability and selecting the next server.
- **Resilience**: Properly handles SIGPIPE signals to prevent server crashes on sudden client disconnects, and manages TCP stream reading iteratively to handle chunked responses.

## Technologies Used
- **C (Standard POSIX)**
- **POSIX Sockets** (`<sys/socket.h>`, `<arpa/inet.h>`)
- **POSIX Threads** (`<pthread.h>`)

## Getting Started

### Prerequisites
- GCC compiler
- UNIX-like operating system (Linux, macOS)

### Compilation
Compile the source code using GCC:
```bash
gcc -o balancer balancer.c -lpthread
```

### Usage
By default, the load balancer listens on port `8080` and routes traffic to three backend servers running on `127.0.0.1` at ports `8001`, `8002`, and `8003`.

1. Start your backend servers (e.g., using Python for quick testing):
   ```bash
   python3 -m http.server 8001 &
   python3 -m http.server 8002 &
   python3 -m http.server 8003 &
   ```

2. Start the load balancer:
   ```bash
   ./balancer
   ```

3. Send traffic to the load balancer:
   ```bash
   curl http://127.0.0.1:8080/
   ```

## Architecture Notes
- The load balancer acts as a transparent TCP proxy. It reads requests from the client and forwards them byte-for-byte to the selected backend.
- It continues reading the response from the backend in a loop until the connection is closed, ensuring complete transmission of fragmented packets.
