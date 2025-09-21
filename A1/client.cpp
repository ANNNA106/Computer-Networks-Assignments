#include <string>
#include <vector>
#include <mutex>
#include <algorithm>
#include <unordered_map>
#include <set>
#include <fstream>
#include <iostream>
#include <sys/socket.h> // For send()
using namespace std;

// Struct to store client information
struct ClientInfo {
    int socket_fd;
    std::string ip_address;
    int port;
    std::string username;
};

// Data Structures
std::unordered_map<std::string, std::set<std::string>> groups; // Group management
std::unordered_map<std::string, int> onlineUsers; // Username -> Socket mapping
std::vector<ClientInfo> connected_clients;       // List of connected clients
std::mutex serverMutex;                          // Mutex to ensure thread safety

// Utility function to send a message to a specific client
void sendMessage(int clientSocket, const std::string& message) 
{
    if(send(clientSocket, message.c_str(), message.size(), 0) < 0)
    {
        std::cerr << "Failed to send message to client." << std::endl;
    }

  
}
void broadcastMessage(const std::string& sender, const std::string& message) {
    std::lock_guard<std::mutex> lock(serverMutex);
    for (const auto& [username, socket] : onlineUsers) {
        if (username != sender) {
            sendMessage(socket, message);
        }
    }
}

// Function to add a client to the system
void addClient(int client_socket_fd, const std::string& ip_address, int port, const std::string& username) {
    std::lock_guard<std::mutex> lock(serverMutex);
   
    if (onlineUsers.find(username) == onlineUsers.end()) {
        onlineUsers[username] = client_socket_fd; // Add client to onlineUsers
        connected_clients.push_back({client_socket_fd, ip_address, port, username}); // Add to connected_clients
        std::cout << "Client " << username << " added successfully with socket " << client_socket_fd << std::endl;
    } else {
        std::cout << "Client " << username << " already exists." << std::endl;
    }
}

// Function to remove a client from the system
void removeClient(int client_socket_fd) 
{
    std::lock_guard<std::mutex> lock(serverMutex);
     string username ="";
    // Find the client in the onlineUsers map and remove it
    for (auto it = onlineUsers.begin(); it != onlineUsers.end(); ++it) {
        if (it->second == client_socket_fd) {
            std::cout << "Client " << it->first << " disconnected." << std::endl;
            username = it->first;
            onlineUsers.erase(it);
            break;
        }
    }

    // Remove the client from the connected_clients vector
    for(auto it = connected_clients.begin(); it != connected_clients.end(); ++it)
    {
        if(it->socket_fd == client_socket_fd)
        {
            connected_clients.erase(it);
            break;
        }
    }
    for (const auto& [user, socket] : onlineUsers) {
        if (user != username) {
            sendMessage(socket, username + " has left the chat.");
        }
    }

}


// Function to create a group
void createGroup(int clientSocket, const string& username, string& groupName) {
    std::lock_guard<std::mutex> lock(serverMutex);
    if (groups.find(groupName) != groups.end()) {
        sendMessage(clientSocket, "Group " + groupName + " already exists.\n");
    } else 
    {
        if(groupName.empty())
        {
            sendMessage(clientSocket, "Error: Group name is empty.\n");
            std::cerr << "Error: Received an empty group name." << std::endl;
            return;
        }
        groups[groupName] = {};
        groups[groupName].insert(username);
        cout << "Debug: Group '" << groupName << "' created. Members: " << username << endl;
        sendMessage(clientSocket, "Group " + groupName + " created successfully. " + username + " has been added to it.\n");
    }
}


// Function to join a group
void joinGroup(int clientSocket, const std::string& username, const std::string& groupName) {
    std::lock_guard<std::mutex> lock(serverMutex);
    if (groups.find(groupName) == groups.end()) {
        sendMessage(clientSocket, "Group " + groupName + " does not exist.\n");
    } else 
    {
        if(groups[groupName].find(username) != groups[groupName].end())
        {
            sendMessage(clientSocket, "You are already part of group " + groupName + ".\n");
            return;
        }
        groups[groupName].insert(username);
        sendMessage(clientSocket, "You joined group " + groupName + ".\n");
    }
}
void sendGroupMessage(int clientSocket, const std::string& username, const std::string& groupName, const std::string& message) {
    std::lock_guard<std::mutex> lock(serverMutex);

    if (groupName.empty()) {
        sendMessage(clientSocket, "Error: Group name is empty.\n");
        std::cerr << "Error: Received an empty group name." << std::endl;
        return;
    }

    std::cout << "Debug: Received group name -> '" << groupName << "'" << std::endl;

    if (groups.find(groupName) == groups.end()) {
        sendMessage(clientSocket, "Group '" + groupName + "' does not exist.\n");
        return;
    }

    if (groups[groupName].find(username) == groups[groupName].end()) {
        sendMessage(clientSocket, "You are not part of group " + groupName + ".\n");
        return;
    }

    for (const std::string& member : groups[groupName]) {
        if (onlineUsers.find(member) != onlineUsers.end()) {
            sendMessage(onlineUsers[member], "[Group " + groupName + "] " + username + ": " + message + "\n");
        }
    }
}

// Function to leave a group
void leaveGroup(int clientSocket, const std::string& username, const std::string& groupName) {
    std::lock_guard<std::mutex> lock(serverMutex);
    if (groups.find(groupName) == groups.end() || groups[groupName].find(username) == groups[groupName].end()) {
        sendMessage(clientSocket, "You are not part of group " + groupName + ".\n");
    } else {
        groups[groupName].erase(username);
        sendMessage(clientSocket, "You left group " + groupName + ".\n");
        sendGroupMessage(clientSocket, username, groupName, "User " + username + " left the group.\n");
    }
}



// Function to broadcast a message to all online users (except sender)

// Function to send a private message
void sendPrivateMessage(int clientSocket, const string& username,const std::string& target, const std::string& message) {
    std::lock_guard<std::mutex> lock(serverMutex);
    if (onlineUsers.find(target) != onlineUsers.end()) {
        int targetSocket = onlineUsers[target];
        sendMessage(targetSocket, username+": " + message + "\n");
    } else {
        sendMessage(clientSocket, "User " + target + " is not online.\n");
    }
}

// Function to handle an individual client
void welcomeClient(int client_socket_fd) {
    // Example: Placeholder logic for handling a client
    std::string welcomeMessage = "Welcome to the chat server!\n";
    sendMessage(client_socket_fd, welcomeMessage);
    // Add more client-handling logic here (e.g., receiving messages, commands)
}

std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\n\r");
    if (first == std::string::npos) return ""; // Empty string
    size_t last = str.find_last_not_of(" \t\n\r");
    return str.substr(first, (last - first + 1));
}

//load users
std::unordered_map<std::string, std::string> loadUsers(const std::string& filename) {
    std::unordered_map<std::string, std::string> users;
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Failed to open the file: " << filename << std::endl;
        return users;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t delimiter_pos = line.find(':');
        if (delimiter_pos != std::string::npos) {
            std::string username = line.substr(0, delimiter_pos);  // Extract username
            std::string password = line.substr(delimiter_pos + 1);  // Extract password
            password = trim(password);
            users[username] = password;
        }
    }

    file.close();
    return users;
}

