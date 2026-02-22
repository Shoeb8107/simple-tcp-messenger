#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <vector>
#include <string>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

namespace
{
    constexpr int buffer_size = 512;
    constexpr int maxClients = 5;
    std::mutex clientsMutex;
    UINT nextClientID = 0;
}

struct clientInfo
{
    UINT clientID;
    std::string name;
    SOCKET clientSocket;
    std::thread clientThread;
    bool connected;
};


std::vector<clientInfo*> clients;

// send a string to a socket
bool sendToSocket(SOCKET s, const std::string& msg)
{
    int sent = send(s, msg.c_str(), static_cast<int>(msg.size()), 0);
    return sent != SOCKET_ERROR;
}

// broadcast message to all connected clients
void broadcastMessage(const std::string& msg, UINT excludeID = 0)
{
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto* client : clients)
    {
        if (client->connected && client->clientID != excludeID)
        {
            sendToSocket(client->clientSocket, msg);
        }
    }
}

// Find client by the name
clientInfo* findClientByName(const std::string& clientName)
{
    for (auto* client : clients)
    {
        if (client->name == clientName)
        {
            return client;
        }
    }
    return nullptr;
}

// Remove a client by ID
void removeClient(UINT clientID)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    clients.erase(std::remove_if(
        clients.begin(),
        clients.end(),
        [clientID](clientInfo* client) { return client->clientID == clientID; }),
        clients.end()
    );
}

void clientHandler(clientInfo* client)
{
    char recvbuf[buffer_size];

    // Recieve the clients chosen name
    int nameBytes = recv(client->clientSocket, recvbuf, buffer_size - 1, 0);
    if (nameBytes <= 0)
    {
        std::cout << "Client disconnected before sending name" << std::endl;
        client->connected = false;
        closesocket(client->clientSocket);
        removeClient(client->clientID);
        return;
    }

    recvbuf[nameBytes] = '\0';
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        client->name = std::string(recvbuf);
    }

    std::cout << "[Server] " << client->name << " has joined the chat!" << std::endl;
    broadcastMessage("[Server] " + client->name + " has joined the chat!'\n'");
    sendToSocket(client->clientSocket, "[Server] Welcome, " + client->name + "!\n");

    // main recieve loop
    while (client->connected)
    {
        int bytesRecieved = recv(client->clientSocket, recvbuf, buffer_size - 1, 0);

        if (bytesRecieved > 0)
        {
            recvbuf[bytesRecieved] = '\0';
            std::string message(recvbuf);

            // client wants to exit
            if (message == "/exit")
            {
                std::cout << "[Server] " << client->name << " has left the chat" << std::endl;
                broadcastMessage("[Server] " + client->name + " has left the chat\n", client->clientID);
                break;
            }
            // client whisper
            else if (message.rfind("/w ", 0) == 0)
            {
                size_t firstSpace = message.find(' ', 3);
                if (firstSpace != std::string::npos)
                {
                    std::string targetName = message.substr(3, firstSpace - 3);
                    std::string whisperMsg = message.substr(firstSpace + 1);

                    std::lock_guard<std::mutex> lock(clientsMutex);
                    clientInfo* targetClient = findClientByName(targetName);
                    if (targetClient != nullptr)
                    {
                        sendToSocket(targetClient->clientSocket, "[Whisper from " + client->name + "] " + whisperMsg + "\n");
                        sendToSocket(client->clientSocket, "[Whisper to " + targetName + "] " + whisperMsg + "\n");
                    }
                    else
                    {
                        sendToSocket(client->clientSocket, "[Server] User '" + targetName + "' not found\n");
                    }
                }
                else
                {
                    sendToSocket(client->clientSocket, "[Server] Usage: /w [username] [message]\n");
                }
            }
            // Normal message to broadcast to everyone
            else
            {
                std::string formatted = client->name + ": " + message + "\n";
                std::cout << formatted;
                broadcastMessage(formatted, client->clientID);
            }
        }
        else if (bytesRecieved == 0)
        {
            std::cout << "[Server] recv failed for " << client->name << ": " << WSAGetLastError() << std::endl;
            break;
        }
    }

    // Cleanup
    client->connected = false;
    closesocket(client->clientSocket);
    removeClient(client->clientID);
}

// Server input handler for global messages, whispers and user kicking
void serverInputHandler()
{
    std::string input;
    while (true)
    {
        std::getline(std::cin, input);
        if (input.empty()) continue;

        // /kick username
        if (input.rfind("/kick", 0) == 0)
        {
            std::string targetClient = input.substr(6);
            std::lock_guard<std::mutex> lock(clientsMutex);
            clientInfo* client = findClientByName(targetClient);
            if (client != nullptr)
            {
                sendToSocket(client->clientSocket, "[Server] You have been kicked from the chat\n");
                std::cout << "[Server] Kicked: " << targetClient << std::endl;
                broadcastMessage("[Server] " + targetClient + " has been kicked from the chat\n", client->clientID);
                client->connected = false;
                shutdown(client->clientSocket, SD_BOTH);
                closesocket(client->clientSocket);
            }
            else
            {
                std::cout << "[Server] User '" << targetClient << "' not found" << std::endl;
            }
        }
        else if (input.rfind("/w ", 0) == 0)
        {
            size_t firstSpace = input.find(' ', 3);
            if (firstSpace != std::string::npos)
            {
                std::string targetName = input.substr(3, firstSpace - 3);
                std::string whisperMsg = input.substr(firstSpace + 1);

                std::lock_guard<std::mutex> lock(clientsMutex);
                clientInfo* targetClient = findClientByName(targetName);
                if (targetClient != nullptr)
                {
                    sendToSocket(targetClient->clientSocket, "[Whisper from Server] " + whisperMsg + "\n");
                    std::cout << "[Whisper to " << targetName << "] " << whisperMsg << std::endl;
                }
                else
                {
                    std::cout << "[Server] User '" << targetName << "' not found." << std::endl;
                }
            }
            else
            {
                std::cout << "[Server] Usage: /w <username> <message>" << std::endl;
            }
        }
        else if (input.rfind("/list", 0) == 0)
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            std::cout << "Connected clients: (" << clients.size() << "/" << maxClients << "):" << std::endl;
            for (auto& client : clients)
            {
                std::cout << "  - " << client->name << " (ID: " << client->clientID << ")" << std::endl;
            }
        }
        // Global broadcast
        else
        {
            std::string formatted = "[Server] " + input + "\n";
            std::cout << formatted;
            broadcastMessage(formatted);
        }
    }
}

int main()
{
    // Initialize WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

    if (result != 0)
    {
        std::cerr << "WSAStartup Failed: " << result << std::endl;
        return 1;
    }
    std::cout << "[Server] Winsock startup successful!" << std::endl;

    // Create a TCP Socket
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }


    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    sockaddr_in service{};
    service.sin_family = AF_INET;
    service.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(55555);

    // Bind the TCP socket with the address info
    if (bind(listenSocket, (SOCKADDR*)&service, sizeof(service)) == SOCKET_ERROR)
    {
        std::cerr << "Socket bind failed! " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }
    
    if (listen(listenSocket, maxClients) == SOCKET_ERROR)
    {
        std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "[Server] Listening on port 55555..." << std::endl;

    // start the server input thread
    std::thread inputThread(serverInputHandler);
    inputThread.detach();

    // Accept loop
    while (true)
    {
        SOCKET acceptSocket = accept(listenSocket, nullptr, nullptr);
        if (acceptSocket == INVALID_SOCKET)
        {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            continue;
        }
        
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            if ((int)clients.size() >= maxClients)
            {
                std::string full = "[Server] Server is full. Try again later \n";
                send(acceptSocket, full.c_str(), (int)full.size(), 0);
                closesocket(acceptSocket);
                std::cout << "[Server] Rejected connection (server full)" << std::endl;
                continue;
            }
        }

        nextClientID++;
        clientInfo* newClient = new clientInfo();
        newClient->clientID = nextClientID;
        newClient->name = "Unknown User";
        newClient->clientSocket = acceptSocket;
        newClient->connected = true;

        // Start the handler thread, add client to clients vector
        newClient->clientThread = std::thread(clientHandler, newClient);
        newClient->clientThread.detach();

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(newClient);
        }

        std::cout << "[Server] New connection accepted (ID: " << newClient->clientID << ")" << std::endl;
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;

}