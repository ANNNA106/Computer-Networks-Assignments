/**
 * TCP Handshake Client Implementation using Raw Sockets
 * CS425: Computer Networks - Assignment 3
 * 
 * This program implements the client side of a TCP three-way handshake
 * using raw sockets. It manually constructs IP and TCP headers to establish
 * a connection with a server.
 */

 #include <iostream>
 #include <cstring>
 #include <cstdlib>
 #include <unistd.h>
 #include <arpa/inet.h>      // For inet_addr and related functions
 #include <netinet/ip.h>     // For IP header structure
 #include <netinet/tcp.h>    // For TCP header structure
 #include <sys/socket.h>     // For socket API
 #include <sys/select.h>     // For select() function for timeout
 #include <errno.h>          // For error handling
 
 // Server parameters - defining where to connect
 #define SERVER_PORT 12345   // Port the server is listening on
 #define SERVER_IP "127.0.0.1"  // Server's IP address (localhost)
 
 // Sequence numbers that must match server's expectations
 // These values are determined by analyzing the server code as required by the assignment
 #define CLIENT_INITIAL_SEQ 200  // Initial sequence number sent in SYN packet
 #define CLIENT_ACK_SEQ 600      // Sequence number used in final ACK packet
 
 // Timeout value in seconds
 #define TIMEOUT_SECONDS 5       // Time to wait for server response before timing out
 
 /**
  * Calculate the Internet checksum for IP/TCP headers
  * 
  * @param ptr Pointer to the data to calculate checksum for
  * @param nbytes Number of bytes to calculate checksum for
  * @return The 16-bit checksum value
  */
 unsigned short checksum(unsigned short *ptr, int nbytes) {
     long sum = 0;          // Sum of 16-bit words
     unsigned short oddbyte;
     unsigned short answer;
 
     // Add all 16-bit words together
     while (nbytes > 1) {
         sum += *ptr++;     // Add current word to sum
         nbytes -= 2;       // Move to next word
     }
 
     // Add any remaining byte if present (odd number of bytes)
     if (nbytes == 1) {
         oddbyte = 0;
         *((unsigned char*)&oddbyte) = *(unsigned char*)ptr;  // Get last byte
         sum += oddbyte;    // Add to sum
     }
 
     // Add carry bits back to sum (fold 32-bit sum to 16 bits)
     sum = (sum >> 16) + (sum & 0xffff);  // Add high 16 to low 16
     sum += (sum >> 16);                  // Add carry bit if needed
     answer = (unsigned short)~sum;       // Take one's complement of sum
 
     return answer;  // Return the checksum
 }
 
 int main() {
     // Create a raw socket for TCP protocol
     // SOCK_RAW gives us control over the headers
     int sock = socket(AF_INET, SOCK_RAW, IPPROTO_TCP);
     if (sock < 0) {
         perror("Socket creation failed");
         return -1;
     }
 
     // Tell kernel we'll provide our own IP headers
     // This is essential for raw socket operation
     int one = 1;
     if (setsockopt(sock, IPPROTO_IP, IP_HDRINCL, &one, sizeof(one)) < 0) {
         perror("setsockopt() failed");
         close(sock);
         return -1;
     }
 
     // Buffer for constructing our packet
     char datagram[4096];  // Large enough for any IP packet
     memset(datagram, 0, sizeof(datagram));  // Clear the buffer
 
     // Set up pointers to the IP and TCP headers within our buffer
     struct iphdr *iph = (struct iphdr *)datagram;
     struct tcphdr *tcph = (struct tcphdr *)(datagram + sizeof(struct iphdr));
     
     // Set up the destination address structure
     struct sockaddr_in dest;
     dest.sin_family = AF_INET;
     dest.sin_port = htons(SERVER_PORT);  // Convert port to network byte order
     dest.sin_addr.s_addr = inet_addr(SERVER_IP);  // Convert IP to network byte order
 
     // Fill in the IP Header fields
     iph->ihl = 5;          // IP Header Length (5 * 32-bit words = 20 bytes)
     iph->version = 4;      // IPv4
     iph->tos = 0;          // Type of Service (normal)
     iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));  // Total packet length
     iph->id = htons(54321);  // ID field (arbitrary)
     iph->frag_off = 0;     // No fragmentation
     iph->ttl = 64;         // Time To Live (64 hops)
     iph->protocol = IPPROTO_TCP;  // Using TCP protocol
     iph->check = 0;        // Initialize checksum to 0 before calculation
     iph->saddr = inet_addr("127.0.0.1");  // Source IP (client)
     iph->daddr = dest.sin_addr.s_addr;    // Destination IP (server)
     
     // Calculate IP header checksum
     iph->check = checksum((unsigned short *)datagram, iph->tot_len);
 
     // Fill in the TCP Header fields for SYN packet (Step 1 of handshake)
     tcph->source = htons(54321);  // Source port (arbitrary client port)
     tcph->dest = htons(SERVER_PORT);  // Destination port (server port)
     tcph->seq = htonl(CLIENT_INITIAL_SEQ);  // Initial sequence number
     tcph->ack_seq = 0;     // No acknowledgment yet
     tcph->doff = 5;        // TCP Header Length (5 * 32-bit words = 20 bytes)
     tcph->syn = 1;         // SYN flag set (initiating connection)
     tcph->window = htons(8192);  // Window size
     tcph->check = 0;       // Initialize checksum to 0 (not computing TCP checksum in this example)
 
     // Send SYN packet to server (Step 1 of handshake)
     std::cout << "[+] Sending SYN..." << std::endl;
     if (sendto(sock, datagram, sizeof(struct iphdr) + sizeof(struct tcphdr), 0,
                (struct sockaddr *)&dest, sizeof(dest)) < 0) {
         perror("sendto() failed");
         close(sock);
         return -1;
     }
 
     // Set up timeout using select()
     fd_set readfds;
     struct timeval timeout;
     timeout.tv_sec = TIMEOUT_SECONDS;  // Timeout in seconds
     timeout.tv_usec = 0;               // Microseconds part (0)
 
     // Buffer for receiving packets
     char buffer[65536];  // Large buffer for incoming packets
     struct sockaddr_in recv_addr;
     socklen_t recv_len = sizeof(recv_addr);
     
     // Time tracking for timeout
     time_t start_time = time(NULL);
     bool received_syn_ack = false;
     
     // Wait for SYN-ACK response from server (Step 2 of handshake)
     while (time(NULL) - start_time < TIMEOUT_SECONDS) {
         // Set up the file descriptor set for select()
         FD_ZERO(&readfds);
         FD_SET(sock, &readfds);
         
         // Reset timeout for each select call
         timeout.tv_sec = TIMEOUT_SECONDS - (time(NULL) - start_time);
         timeout.tv_usec = 0;
 
         // Wait for data to be available or timeout
         int activity = select(sock + 1, &readfds, NULL, NULL, &timeout);
         
         if (activity < 0) {
             perror("select() failed");
             close(sock);
             return -1;
         }
         
         if (activity == 0) {
             // Timeout occurred
             continue;
         }
         
         // Data is available to read
         if (FD_ISSET(sock, &readfds)) {
             int bytes = recvfrom(sock, buffer, sizeof(buffer), 0,
                                 (struct sockaddr *)&recv_addr, &recv_len);
             if (bytes < 0) {
                 perror("recvfrom() failed");
                 continue;
             }
 
             // Extract headers from received packet
             struct iphdr *recv_ip = (struct iphdr *)buffer;
             struct tcphdr *recv_tcp = (struct tcphdr *)(buffer + recv_ip->ihl * 4);  // IP header length can vary
 
             // Validate that this is the SYN-ACK we're expecting
             // Check: correct source port, destination port, SYN and ACK flags, and expected ACK number
             if (recv_tcp->source == htons(SERVER_PORT) &&    // From server port
                 recv_tcp->dest == htons(54321) &&            // To our client port
                 recv_tcp->syn == 1 && recv_tcp->ack == 1 &&  // Both SYN and ACK flags set
                 ntohl(recv_tcp->ack_seq) == CLIENT_INITIAL_SEQ + 1) {  // ACK = our SEQ + 1
 
                 // Found our SYN-ACK response
                 std::cout << "[+] Received SYN-ACK. SEQ: " << ntohl(recv_tcp->seq)
                         << ", ACK: " << ntohl(recv_tcp->ack_seq) << std::endl;
                 
                 received_syn_ack = true;
 
                 // Prepare ACK packet (Step 3 of handshake)
                 memset(datagram, 0, sizeof(datagram));  // Clear buffer
                 
                 // Reset pointers to headers in our buffer
                 iph = (struct iphdr *)datagram;
                 tcph = (struct tcphdr *)(datagram + sizeof(struct iphdr));
 
                 // Fill in IP header again for ACK packet
                 iph->ihl = 5;
                 iph->version = 4;
                 iph->tos = 0;
                 iph->tot_len = htons(sizeof(struct iphdr) + sizeof(struct tcphdr));
                 iph->id = htons(54321);
                 iph->frag_off = 0;
                 iph->ttl = 64;
                 iph->protocol = IPPROTO_TCP;
                 iph->saddr = inet_addr("127.0.0.1");  // Client IP
                 iph->daddr = dest.sin_addr.s_addr;    // Server IP
                 iph->check = checksum((unsigned short *)datagram, iph->tot_len);
 
                 // Fill in TCP header for ACK packet
                 tcph->source = htons(54321);        // Client port
                 tcph->dest = htons(SERVER_PORT);    // Server port
                 tcph->seq = htonl(CLIENT_ACK_SEQ);  // Our ACK sequence number
                 tcph->ack_seq = htonl(ntohl(recv_tcp->seq) + 1);  // Server's SEQ + 1
                 tcph->doff = 5;
                 tcph->ack = 1;     // Set ACK flag (final step of handshake)
                 tcph->window = htons(8192);
                 tcph->check = 0;   // TCP checksum (not computed in this example)
 
                 // Send ACK packet to server (Step 3 of handshake)
                 std::cout << "[+] Sending ACK..." << std::endl;
                 if (sendto(sock, datagram, sizeof(struct iphdr) + sizeof(struct tcphdr), 0,
                         (struct sockaddr *)&dest, sizeof(dest)) < 0) {
                     perror("sendto() failed (ACK)");
                     close(sock);
                     return -1;
                 }
 
                 // Handshake is now complete
                 std::cout << "[+] Handshake complete." << std::endl;
                 break;
             }
             // If not our SYN-ACK, continue waiting
         }
     }
     
     // Check if we timed out waiting for SYN-ACK
     if (!received_syn_ack) {
         std::cerr << "[-] ERROR: Timeout waiting for server response." << std::endl;
         std::cerr << "[-] Make sure the server is running at " << SERVER_IP << ":" << SERVER_PORT << std::endl;
     }
 
     // Clean up and exit
     close(sock);
     return received_syn_ack ? 0 : -1;  // Return success only if handshake completed
 }