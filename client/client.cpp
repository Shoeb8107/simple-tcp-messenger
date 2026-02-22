#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <chrono>
#include <thread>
#include <atomic>
#include <signal.h>
#include <cstdio>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

namespace
{
    std::chrono::seconds timespan(1);
    constexpr int buffer_size = 512;
    std::atomic<bool> running{ false };
    SOCKET clientSocket = INVALID_SOCKET;
}

// User can exit using CTRL+C
void handleExit(int signal)
{
    running = false;
    return;
}

void sendMessage(SOCKET clientSocket)
{
    while (running)
    {
        std::cout << "You: ";
        char msg[buffer_size];
        std::cin.getline(msg, buffer_size);
        if (std::cin.fail() || running == false) { break; }
        send(clientSocket, msg, strlen(msg), 0);
        if (strstr(msg, "/exit"))
        {
            running = false;
            // we need to handle the exit gracefully here. Maybe use the handleExit()?
        }

    }
}

void recieveMessage(SOCKET clientSocket)
{
    char buf[buffer_size];
    while (running)
    {
        int recv_result = recv(clientSocket, buf, buffer_size-1, 0);
        if (recv_result > 0)
        {
            buf[recv_result] = '\0';
            std::cout << "Message received: " << buf << std::endl;
        }
        else if (recv_result == 0)
        {
            std::cout << "Connection closed!" << std::endl;
            running = false;
            break;

        }
        else
        {
            std::cerr << "Attempting to receive message failed: " << WSAGetLastError() << std::endl;
            running = false;
            break;
            // Might also need an fflush(stdout)
        }
        fflush(stdout);
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
    int iFamily = AF_INET;
    int iType = SOCK_STREAM;
    int iProtocol = IPPROTO_TCP;

    clientSocket = socket(iFamily, iType, iProtocol);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket function called with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::this_thread::sleep_for(timespan);
    std::cout << "Socket function succeeded!" << std::endl;

    sockaddr_in clientService;
    clientService.sin_family = AF_INET;
    clientService.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    clientService.sin_port = htons(55555);

    int connect_result = connect(clientSocket, (SOCKADDR*)&clientService, sizeof clientService);
    if (connect_result == SOCKET_ERROR)
    {
        std::cerr << "Connection to host failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    running = true;
    std::cout << "Connected to host!" << std::endl;
    // Think about SetConsoleCtrlHandler() instead of signal()
    signal(SIGINT, handleExit);

    // Enter name
    char name[buffer_size];
    std::cout << "Enter your username: ";
    std::cin.getline(name, buffer_size);
    send(clientSocket, name, sizeof(name), 0);

    //Create separate threads for sending and recieving messages
    std::thread sendThread(sendMessage, clientSocket);
    std::thread recieveThread(recieveMessage, clientSocket);

    if (sendThread.joinable()) sendThread.join();
    shutdown(clientSocket, SD_BOTH);
    if (recieveThread.joinable()) recieveThread.join();

    closesocket(clientSocket);
    WSACleanup();

}