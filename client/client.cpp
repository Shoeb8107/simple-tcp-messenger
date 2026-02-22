#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <atomic>
#include <string>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")
#pragma warning(disable:4996)

namespace
{
    constexpr int buffer_size = 512;
    std::atomic<bool> running{ false };
    SOCKET clientSocket = INVALID_SOCKET;
}

BOOL WINAPI ConsoleHandler(DWORD event)
{
    switch (event)
    {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
        std::cout << "\nDisonncting..." << std::endl;

        if (clientSocket != INVALID_SOCKET)
        {
            std::string exitMsg = "/exit";
            send(clientSocket, exitMsg.c_str(), (int)exitMsg.size(), 0);
            shutdown(clientSocket, SD_BOTH);
        }

        running = false;
        Sleep(500);
        return TRUE;
    default:
        return FALSE;
    }
}

void sendMessage()
{
    std::string input;
    while (running)
    {
        std::getline(std::cin, input);
        if (!running || std::cin.fail()) break;
        if (input.empty()) continue;

        send(clientSocket, input.c_str(), (int)input.size(), 0);

        if (input == "/exit")
        {
            std::cout << "Disconnecting..." << std::endl;
            running = false;
            break;
        }
    }
}


void receiveMessage()
{
    char buf[buffer_size];
    while (running)
    {
        int bytedReceived = recv(clientSocket, buf, buffer_size - 1, 0);
        if (bytedReceived > 0)
        {
            buf[bytedReceived] = '\0';
            std::cout << buf;
            // Reprint the prompt after incoming message
            fflush(stdout);
        }
        else
        {
            if (running)
            {
                if (bytedReceived == 0)
                {
                    std::cout << "\n[Disconnected from server]" << std::endl;
                }
                else
                {
                    std::cerr << "\n[Connection error: " << WSAGetLastError() << "]" << std::endl;
                }
            }
            running = false;
            break;
        }
    }
}

int main()
{
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup Failed" << std::endl;
        return 1;
    }

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET)
    {
        std::cerr << "Socket function called with error: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
    serverAddr.sin_port = htons(55555);

    if(connect(clientSocket, (SOCKADDR*)&serverAddr, sizeof serverAddr) == SOCKET_ERROR)
    {
        std::cerr << "Connection to host failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Connected to host!" << std::endl;
    running = true;

    // Register the console control handler
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    // Send the client username as the first message
    std::string name;
    std::cout << "Enter your username: ";
    std::getline(std::cin, name);
    send(clientSocket, name.c_str(), (int)name.size(), 0);

    std::cout << "\n=== Chat started. Commands: ===" << std::endl;
    std::cout << "   /w [user] [msg]   - Whisper" << std::endl;
    std::cout << "   /exit             - Disconnect\n" << std::endl;


    //Create separate threads for sending and recieving messages
    std::thread sendThread(sendMessage);
    std::thread receiveThread(receiveMessage);

    if (sendThread.joinable()) sendThread.join();
    shutdown(clientSocket, SD_BOTH);
    if (receiveThread.joinable()) receiveThread.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}