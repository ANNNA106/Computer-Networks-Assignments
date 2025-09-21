#include"server.cpp"
#include <string.h>
#include"request.cpp"
#include"response.cpp"
#include<thread>
#include<unordered_map>
#include<arpa/inet.h>
#include<unistd.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<sys/types.h>
#include<iostream>
#include<fstream>
#include"client.cpp"

// remember to add in readme that we can only keep one word group name
// if close server with Ctrl+C, it's not closing properly
// sockets are not resued
// join_group with empty name gives segmentation fault -done
// if after closing a group, we try to send message to that group, it gives segmentation fault
// if we try to send message to a group that doesn't exist, it gives segmentation fault
// if we close a client and then close the server it doesn't bind again
// if we close a client and then try to send message to that client, it gives segmentation fault
//

// do client socket mapping beforehand
// closing a server is not closing the clients


//utility function to log the serving of a file.

// void handleSignal(int signal) {
//     if (signal == SIGINT) {
//         std::cout << "\n[INFO] Server shutting down gracefully..." << std::endl;
//         serverRunning = false;
        
//         // Close all client connections
//         std::lock_guard<std::mutex> lock(clientSocketsMutex);
//         for (int socket_fd : clientSockets) {
//             close(socket_fd);
//         }
//         clientSockets.clear();

//         exit(0);
//     }
// }

void logServingFile(const std::string& path, const std::string& mimetype) {
    std::cout << "Serving file: " << path << " with MIME type: " << mimetype << std::endl;
}

void handleClient(int client_socket_fd, const std::unordered_map<std::string, std::string>& users) {
    char buffer[1024];
    std::string username, password;

    // Step 1: Authenticate the user
    std::string username_prompt = "Enter your username: ";
    sendMessage(client_socket_fd, username_prompt);

    ssize_t bytes_received = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive username from client." << std::endl;
        close(client_socket_fd);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate
    username = std::string(buffer);

    std::string password_prompt = "Enter your password: ";
    sendMessage(client_socket_fd, password_prompt);


    bytes_received = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        std::cerr << "Failed to receive password from client." << std::endl;
        close(client_socket_fd);
        return;
    }
    buffer[bytes_received] = '\0'; // Null-terminate
    password = std::string(buffer);

    if (users.find(username) == users.end() || users.at(username) != password) {
        std::string auth_failed = "Authentication failed. Disconnecting.\n";
        sendMessage(client_socket_fd, auth_failed);
        close(client_socket_fd);
        return;
    }
    // Step 2: Add the client
    sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    getpeername(client_socket_fd, (struct sockaddr*)&client_addr, &addr_len);
    addClient(client_socket_fd, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port), username);

    welcomeClient(client_socket_fd);
    broadcastMessage(username,  username + " has joined the chat.");
    // Step 3: Handle client commands
    while (true) {
        bytes_received = recv(client_socket_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            std::cerr << "Client disconnected: " << username << std::endl;
            break;
        }

        buffer[bytes_received] = '\0'; // Null-terminate
        string clientMessage = string(buffer);

        // Parse client message using HttpRequest
        HttpRequest req;
        int parsed = req.parseRequest(clientMessage);
        if(parsed<0){
            sendMessage(client_socket_fd, "Invalid command/syntax\n");
            continue;
        }

        // Command handling based on parsed request
        if (req.method == "/create_group") 
        {
            createGroup(client_socket_fd,username, req.target);

        } else if (req.method == "/join_group") {
            joinGroup(client_socket_fd, username, req.target);

        } else if (req.method == "/leave_group") {
            leaveGroup(client_socket_fd, username, req.target);

        } else if (req.method == "/group_msg") {
            std::string groupName = req.target;
            std::string message = req.headers;
            sendGroupMessage(client_socket_fd, username, groupName, message);

        } else if (req.method == "/broadcast") {
            std::string message = req.headers;
            broadcastMessage(username, username + ": " + message);

        } else if (req.method == "/msg") {
            std::string target = req.target;
            std::string message = req.headers;
            for(char c: message){
                std::cout<<(int)c<<" ";
            }
            cout<<endl;
            sendPrivateMessage(client_socket_fd, username,target, message);

        } else if (req.method == "/exit") {
            sendMessage(client_socket_fd, "Goodbye!\n");
            break;

        } else {
            sendMessage(client_socket_fd, "Unknown command. Available commands: /create_group, /join_group, /leave_group, /group_msg, /broadcast, /msg, /exit.\n");
        }
    }

    // Step 4: Remove the client and close the connection
    removeClient(client_socket_fd);
    close(client_socket_fd);
}


int main()
{
    Server server = Server(8080);
    struct sockaddr_in client_addr;
    socklen_t client_addr_size;
    int s_fd;
    s_fd = server.start();
    client_addr_size = sizeof(struct sockaddr_in); 
  std::unordered_map<std::string, std::string> users = loadUsers("users.txt"); 
//   users["alice"]="password123";
//     users["bob"]="qwerty456";
//     users["charlie"]="secure789";
//     users["dave"]="helloWorld!";
//     users["eve"]="trustno1";
//     users["frank"]="letmein";
//     users["grace"]="passw0rd";


    while(1){
        //handle client requests.
        //Accept system call .
          int client_socket_fd = accept(s_fd, (struct sockaddr*)&client_addr, &client_addr_size);
          
          if (client_socket_fd < 0) {
            std::cerr << "Failed to accept client request." << std::endl;
            continue; // Do not terminate the server, continue accepting other clients
        }
         try 
         {
             
            std::thread clientThread([client_socket_fd, &users]() {
                handleClient(client_socket_fd, users);
            });
            clientThread.detach();
        } catch (const std::exception& e) {
            std::cerr << "Error creating thread: " << e.what() << std::endl;
            close(client_socket_fd);
        }
    }
    
}
