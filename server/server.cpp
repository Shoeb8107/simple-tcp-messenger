#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <mutex>
#include <vector>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

namespace
{
    constexpr int buffer_size = 512;
    std::mutex clientsMutex;
    UINT clientID = 0;
}

struct clientInfo
{
    UINT clientID;
    std::string name;
    SOCKET clientSocket;
    std::thread clientThread;
};
std::vector<clientInfo> clients;

void setClientName(UINT clientID, char name[])
{
    for (int i = 0; i < clients.size(); ++i)
    {
        if (clients[i].clientID == clientID)
        {
            clients[i].name = std::string(name);
        }
    }
}

void clientHandler(SOCKET clientSocket, UINT clientID)
{
    char name[buffer_size];
    char msg[buffer_size];
    recv(clientSocket, name, strlen(name), 0);
    setClientName(clientID, name);

    std::cout << "Welcome "<< name << " !" << std::endl;
    // Send some data to client
    //const char* buf = "Hello from Server!";
    //int sentData = send(clientSocket, buf, strlen(buf), 0);
    //if (sentData == SOCKET_ERROR)
    //{
    //    std::cerr << "Failed to send message to client: " << WSAGetLastError() << std::endl;
    //    return;

    //}
    //std::cout << "Message sent!" << std::endl;




    // Keep listening for client response
    char recvbuf[buffer_size];
    while (true)
    {
        int bytesReceived = recv(clientSocket, recvbuf, sizeof(recvbuf), 0);

        if (bytesReceived > 0)
        {
            recvbuf[bytesReceived] = '\0';
            std::cout << "Recieved from client: " << recvbuf << std::endl;
        }
        else if (bytesReceived == 0)
        {
            std::cout << "Client Disconnected!" << std::endl;
            break;
        }
        else
        {
            std::cerr << "Recv failed:" << WSAGetLastError() << std::endl;
            break;
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
    std::cout << "Winsock startup successful!" << std::endl;

    // Create a TCP Socket
    SOCKET sock = INVALID_SOCKET;
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    sock = socket(iFamily, iType, iProtocol);
    if (sock == INVALID_SOCKET)
    {
        std::cerr << "Socket function called with error: " << WSAGetLastError() << std::endl;
        return 1;
    }
    std::cout << "Socket function succeeded!" << std::endl;

    // The sockaddr_in structure specifies the address family,
    // IP address, and port for the socket that is being bound.
    sockaddr_in service;
    service.sin_family = AF_INET;
    service.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    service.sin_port = htons(55555);

    // Bind the TCP socket with the address info
    int bind_result = bind(sock, (SOCKADDR*)&service, sizeof(service));
    if (bind_result == SOCKET_ERROR)
    {
        std::cerr << "Socket bind failed! " << WSAGetLastError() << std::endl;
    }
    std::cout << "Socket bind succeeded!" << std::endl;

    // Set the socket to a listening state
    if (listen(sock, 1) == SOCKET_ERROR)
    {
        std::cerr << "Listen function failed with error: " << WSAGetLastError() << std::endl;
    }
    std::cout << "Socket is listening" << std::endl;

    // Create a socket to accept incoming connections
    SOCKET acceptSocket;
    std::cout << "Waiting for client to connect..." << std::endl;

    // Accept the connection and add socket to client vector
    acceptSocket = accept(sock, nullptr, nullptr);
    if (acceptSocket == INVALID_SOCKET)
    {
        std::cerr << "Failed to accept client connection: " << WSAGetLastError() << std::endl;
        closesocket(acceptSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connection to client established" << std::endl;

    clientID++;
    std::thread t(clientHandler, acceptSocket, clientID);
    std::lock_guard<std::mutex> guard(clientsMutex);
    std::string randomName = "random";
    clients.push_back({ clientID, randomName, acceptSocket, std::move(t) });

    for (int i = 0; i < clients.size(); ++i)
    {
        if (clients[i].clientThread.joinable())
        {
            clients[i].clientThread.join();
        }
    }

    closesocket(acceptSocket);

    WSACleanup();

}