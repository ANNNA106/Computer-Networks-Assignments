# TCP Handshake Client Implementation

## CS425: Computer Networks - Assignment 3

This project implements the client side of a TCP three-way handshake using raw sockets as required for Assignment 3 of CS425: Computer Networks.

## Overview

This client application demonstrates a low-level implementation of the TCP three-way handshake process by manually constructing and sending IP and TCP packets. It communicates with the server provided in the course repository to complete the handshake sequence with specific sequence numbers as required by the assignment.

The three-way handshake consists of:
1. Client sends SYN packet to server
2. Server responds with SYN-ACK packet
3. Client sends ACK packet to complete the handshake

## Requirements

- Linux operating system
- Root/sudo privileges (required for raw socket operations)
- G++ compiler
- Network libraries (included in standard Linux distributions)

## Setup and Compilation

1. Clone the repository provided in the assignment:
```bash
git clone https://github.com/privacy-iitk/cs425-2025.git
```

2. Navigate to the assignment directory:
```bash
cd cs425-2025/Homeworks/A3
```

3. Compile the server (provided code):
```bash
g++ -o server server.cpp
```

4. Compile our client implementation:
```bash
g++ -o client client.cpp
```

## Running the Application

1. First, start the server in one terminal:
```bash
sudo ./server
```

2. Then, run the client in another terminal:
```bash
sudo ./client
```

Note: Both applications require root privileges to use raw sockets.

## Implementation Details

### Key Constants and Sequence Numbers

The client uses specific sequence numbers to match the server's expectations:
- `CLIENT_INITIAL_SEQ` (200): Initial sequence number sent in the SYN packet
- `CLIENT_ACK_SEQ` (600): Sequence number used in the final ACK packet

These values were determined by analyzing the server code as instructed in the assignment.

### Raw Socket Creation

The client creates a raw socket to gain direct access to the network protocol:

```cpp
int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
```

This allows us to manually construct the IP and TCP headers instead of relying on the operating system's network stack.

### IP Header Inclusion

We inform the kernel that our application will provide its own IP headers:

```cpp
setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one))
```

### TCP Handshake Process

#### Step 1: SYN Packet
The client constructs and sends an IP packet containing a TCP header with the SYN flag set:
- Source IP: 127.0.0.1 (localhost)
- Destination IP: 127.0.0.1 (server address)
- Source Port: 54321 (arbitrary client port)
- Destination Port: 12345 (server port)
- SYN flag: 1
- Sequence Number: 200 (CLIENT_INITIAL_SEQ)

#### Step 2: Receiving SYN-ACK
After sending the SYN packet, the client waits to receive the server's SYN-ACK response. It validates that:
- The packet is from the expected server port (12345)
- Both SYN and ACK flags are set
- The acknowledgment number is CLIENT_INITIAL_SEQ + 1 (201)

#### Step 3: ACK Packet
Upon receiving a valid SYN-ACK, the client sends the final ACK packet:
- Sequence Number: 600 (CLIENT_ACK_SEQ)
- Acknowledgment Number: Server's sequence number + 1
- ACK flag: 1

### Checksum Calculation

The implementation includes a custom checksum function for calculating the Internet checksum required for valid IP and TCP headers:

```cpp
unsigned short checksum(unsigned short *ptr, int nbytes)
```

This function follows the standard Internet checksum algorithm:
1. Sum all 16-bit words
2. Add any carry bits back into the sum
3. Take the one's complement of the result

## Testing

When both the client and server run successfully, you should see output confirming the three-way handshake completion:
1. Client sends SYN
2. Client receives SYN-ACK from server
3. Client sends ACK
4. Handshake complete message

## Learning Outcomes

This implementation demonstrates several important networking concepts:
- Structure and format of IP and TCP headers
- TCP three-way handshake sequence
- Checksum calculation for network packets
- Raw socket programming in C++
- Packet construction and parsing

## Limitations

- This implementation is designed for educational purposes and only performs the handshake, not data transfer
- It requires root privileges, which would not be suitable for production applications
- Fixed to work with the specific server configuration provided in the assignment
- Uses hardcoded sequence numbers and addresses

## Troubleshooting

- If you encounter "Operation not permitted" errors, make sure you're running with sudo
- If the server doesn't respond, check that it's running and listening on port 12345
- Ensure your firewall is not blocking local connections
# Team Members
- Anya Rajan,220191
- Ananya Singh Baghel,220136
- Nandini Akolkar, 220692