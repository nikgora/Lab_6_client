#include <stdio.h>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
#include <string.h>
#include <sstream>
#include "winsock2.h"

#define DEFAULT_BUFLEN 1024



using namespace std;


void clenup(char *recvbuf, int len) {
    for (int i = len; i< strlen(recvbuf);i++){
        recvbuf[i]='\000';
    }
}

struct addrinfo *result = NULL,
        *ptr = NULL,
        hints;


bool BindSocket(SOCKET &DataSocket,string port,char* argv[]){
    int iResult;
    bool q=0;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], port.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return 1;
    }
    DataSocket = INVALID_SOCKET;
    // Attempt to connect to the first address returned by
// the call to getaddrinfo
    ptr=result;
// Create a SOCKET for connecting to server
    DataSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (DataSocket == INVALID_SOCKET) {
        printf("Error at socket(): %ld\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return 1;
    }
    // Connect to server.
    iResult = connect( DataSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        q = 1;
        DataSocket = INVALID_SOCKET;
    }
    // Should really try the next address returned by getaddrinfo
// if the connect call failed

    if (q){
        ptr=ptr->ai_next;
        DataSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        iResult = connect( DataSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
        if (iResult == SOCKET_ERROR) {
            closesocket(DataSocket);
            DataSocket = INVALID_SOCKET;
        }
    }

// But for this simple example we just free the resources
// returned by getaddrinfo and print an error message
    freeaddrinfo(result);
    if (DataSocket == INVALID_SOCKET) {
        printf("Unable to connect to server!\n");
        WSACleanup();
        return 1;
    }
}



int main(int argc, char* argv[]) {
    WSADATA wsaData;
    char recvbuf[DEFAULT_BUFLEN]{};
    int iResult, iSendResult;
    int recvbuflen = DEFAULT_BUFLEN;
    int q =0;
// Initialize Winsock

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    SOCKET ConnectSocket = INVALID_SOCKET;
    BindSocket(ConnectSocket,"12",argv);
    string command;
    printf("Connect to server is successful!\n");
    string directory = "./";
    bool isBinary = false;
    do {
        cout<<"Enter command:\n";
        string line;
        // create a stringstream from the line
        getline(cin,line);
        stringstream ss(line);
        ss>>command;
        // read the first and second words from the stringstream
        iResult=send(ConnectSocket,to_string(command.length()).c_str(),DEFAULT_BUFLEN,0);
        if (iResult < 0) {
            cout<< "recv failed:\n" << WSAGetLastError();
            closesocket(ConnectSocket);
            return 1;

        }
        iResult=send(ConnectSocket,command.c_str(),command.length(),0);
        if (iResult < 0) {
            cout<< "recv failed:\n" << WSAGetLastError();
            closesocket(ConnectSocket);
            return 1;
        }
        if (command == "cd"){
            string newDir;
            ss>>newDir;
            SOCKET DataSocket;
            BindSocket(DataSocket,"13",argv);
            iResult=send(ConnectSocket,to_string(newDir.length()).c_str(),DEFAULT_BUFLEN,0);
            if (iResult < 0) {
                cout<< "recv failed:\n" << WSAGetLastError();
                closesocket(ConnectSocket);
            }
            iResult=send(ConnectSocket,newDir.c_str(),newDir.length(),0);
            if (iResult < 0) {
                cout<< "recv failed:\n" << WSAGetLastError();
                closesocket(ConnectSocket);
            }
        }
        else if (command == "dir"){

        }
        else if (command == "put"){

        }
        else if (command == "get"){

        }
        else if (command == "ascii"){

        }
        else if (command == "binary"){

        }
        else if (command == "user"){

        }
        else if (command == "lcd"){

        }
        else if (command == "pwd"){

        }
        else if (command == "login"){

        }
        else if (command == "password"){

        }
    }while (iResult > 0);
    return 0;
}

