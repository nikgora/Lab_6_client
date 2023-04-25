#include <cstdio>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <sstream>
#include "winsock2.h"
#include "sha-1.h"
#define DEFAULT_BUF_LEN 1024



using namespace std;


void clenup(char *recvbuf, int len) {
    for (int i = len; i< strlen(recvbuf);i++){
        recvbuf[i]='\000';
    }
}

struct addrinfo *result = nullptr,
        *ptr = nullptr,
        hints;

bool Recive(string& res,SOCKET DataSocket){
    char ls[DEFAULT_BUF_LEN];
    int iResult = recv(DataSocket, ls, DEFAULT_BUF_LEN, 0);
    if (iResult < 0) {
        cout << "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;
    }
    int len = stoi(ls);
    char buffer[len];
    iResult = recv(DataSocket, buffer, len, 0);
    if (iResult < 0) {
        cout<<"recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;


    }
    clenup(buffer, len);
    res = buffer;
    return false;
}
bool Send(const string& string, SOCKET DataSocket){
    int iResult=send(DataSocket,to_string(string.length()).c_str(),DEFAULT_BUF_LEN,0);
    if (iResult < 0) {
        cout<< "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;
    }
    iResult=send(DataSocket,string.c_str(),string.length(),0);
    if (iResult < 0) {
        cout<< "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;
    }
    return false;
}

bool BindSocket(SOCKET &DataSocket,const string& port,char* argv[]){
    int iResult;
    bool q=false;
    ZeroMemory( &hints, sizeof(hints) );
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = IPPROTO_TCP;
    // Resolve the server address and port
    iResult = getaddrinfo(argv[1], port.c_str(), &hints, &result);
    if (iResult != 0) {
        printf("getaddrinfo failed: %d\n", iResult);
        WSACleanup();
        return true;
    }
    DataSocket = INVALID_SOCKET;
    // Attempt to connect to the first address returned by
// the call to getaddrinfo
    ptr=result;
// Create a SOCKET for connecting to server
    DataSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (DataSocket == INVALID_SOCKET) {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return true;
    }
    // Connect to server.
    iResult = connect( DataSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        q = true;
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
        return true;
    }
    return false;
}

bool GetBinary(SOCKET socket, string name, const string &second_name, string &error) {
    if (!second_name.empty())name = second_name;
    ofstream outputFile;
    outputFile.open(name, ios::binary);
    if (!outputFile) {
        error = "Error opening file.";
        return true;
    }
    string res;
    if (Recive(res,socket)) return true;
    outputFile.write(res.c_str(), res.length());
    outputFile.close();
    return false;
}

bool Get(SOCKET socket,string name, const string &second_name, string &error) {
    if (!second_name.empty())name = second_name;
    ofstream outputFile;
    outputFile.open(name);
    if (!outputFile) {
        error = "Error opening file.";
        return true;
    }
    string res;
    if (Recive(res, socket))return true;
    outputFile.write(res.c_str(), res.length());
    outputFile.close();
    return false;
}

bool PutBinary(SOCKET socket, const string &name, string &error) {
    ifstream inputFile;
    inputFile.open(name, ios::binary);
    if (!inputFile) {
        error = "Error opening file.";
        return true;
    }
    //get info from socket
    char *l;
    inputFile.seekg(0, ios::end);
    int len = inputFile.tellg();
    inputFile.seekg(0, ios::beg);

    inputFile.read(l, len);
    if (Send(l,socket))return true;
    inputFile.close();
    return false;
}

bool Put(SOCKET socket, const string &name, string &error) {
    ifstream inputFile;
    inputFile.open(name);
    if (!inputFile) {
        error = "Error opening file.";
        return true;
    }
    //get info from socket
    string l;
    string line;
    while (std::getline(inputFile, line)) {
        l += line + "\n";
    }
    if(Send(l,socket))return true;
    inputFile.close();
    return false;
}


int main(int argc, char* argv[]) {
    WSADATA wsaData;
    char recvbuf[DEFAULT_BUF_LEN]{};
    int iResult=0, iSendResult;
    SHA1 nikname,password;
    int recvbuflen = DEFAULT_BUF_LEN;
    int q =0;
// Initialize Winsock

    iResult = WSAStartup(MAKEWORD(2,2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    // Resolve the server address and port

    auto ConnectSocket = INVALID_SOCKET;
    bool err = BindSocket(ConnectSocket,"12",argv);
    if (err){
        return 1;
    }
    string command;
    printf("Connect to server is successful!\n");
    string directory = "./";
    bool isBinary = false;
    int anonymusCode = 0;//0-Not login, 1-anonym mode, 2- user mode
    bool isOpen=false;
    auto DataSocket = INVALID_SOCKET;

    do {
        cout<<"\nEnter command:\n";
        string line;
        // create a stringstream from the line
        getline(cin,line);
        stringstream ss(line);
        ss>>command;

        if (command=="open"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }
            isOpen = true;
            BindSocket(DataSocket,"13",argv);
        }
        else if (command == "lcd"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }
            ss>>directory;
        }
        else if (!isOpen){
            cout<<"Socket must be open\n";
            continue;
        }
        else if (command == "login"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            string nik;
            ss>>nik;
            nikname.update(nik);
            bool anonym=false;
            if(nik=="")
            {
                password.update("");
                anonym = true ;
            }
            else {
                cout << "password: ";
                string pass;
                cin>>pass;
                password.update(pass);
            }
            if (Send(nikname.final(),DataSocket)){
                iResult=-1;
                continue;
            }
            if (Send(password.final(),DataSocket)){
                iResult=-1;
                continue;
            }
            string res;
            if(Recive(res,DataSocket)){
                iResult=-1;
                continue;
            }
            if (res=="Login successful"){
                if (anonym){
                    anonymusCode=1;
                }
                else{
                    anonymusCode=2;
                }
            }
        }
        else if (!anonymusCode){
            cout<<"You must be at least anonym";
            continue;
        }
        else if (command== "close"){
            isOpen=false;
            isBinary=false;
            anonymusCode = 0;
            closesocket(DataSocket);
        }
        else if (command == "cd"){
            // read the first and second words from the stringstream

            string newDir;
            if (Send(newDir,DataSocket)){
                iResult=-1;
                continue;
            }
        }
        else if (command == "dir"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            string file;
            ss>>file;
            if (file=="")file="*";
            if(Send(file,DataSocket)){
                iResult=-1;
                continue;
            }
            string res;
            if(Recive(res,DataSocket)){
                iResult=-1;
                continue;
            }
            cout<<res;
        }
        else if (command == "get"){
            string name, local_name;
            string error;
            ss>>name>>local_name;
            if(Recive(name,DataSocket)){iResult=-1;
                continue;}
            if(Recive(local_name,DataSocket)){iResult=-1;
                continue;}
            if(isBinary){
                if(GetBinary(DataSocket,local_name,local_name,error)){iResult=-1;
                    cout<<error;continue;}
            }
            else{
                if(Get(DataSocket,local_name,local_name,error)){iResult=-1;
                    cout<<error;
                    continue;}

            }

        }
        else if (command == "ascii"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            isBinary= false;
        }
        else if (command == "binary"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            isBinary= true;
        }
        else if (command == "user"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            string nik;
            ss>>nik;
            nikname.update(nik);
            bool anonym=false;
            if(nik=="")
            {
                password.update("");
                anonym = true ;
            }
            else {
                cout << "password: ";
                string pass;
                cin>>pass;
                password.update(pass);
            }
            if (Send(nikname.final(),DataSocket)){
                iResult=-1;
                continue;
            }
            if (Send(password.final(),DataSocket)){
                iResult=-1;
                continue;
            }
            string res;
            if(Recive(res,DataSocket)){
                iResult=-1;
                continue;
            }
            if (res=="Login successful"){
                if (anonym){
                    anonymusCode=1;
                }
                else{
                    anonymusCode=2;
                }
            }
        }

        else if (command == "pwd"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            string res;
            Recive(res,DataSocket);
            cout<<res;
        }/*
        else if (command == "password"){
            SHA1 sha1;
            string pass;
            ss>>pass;
            sha1.update(pass);
            if(Send(sha1.final(),DataSocket)){
                iResult=-1;
                continue;
            }
        }*/
        else if (command=="quit"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            iResult=-1;
            isOpen=false;
            isBinary=false;
            anonymusCode = 0;
            closesocket(DataSocket);
        }
        else if (anonymusCode<2){
            cout<<"You must be login as user\n";
            continue;
        }
        else if (command == "put"){
            if(Send(command,ConnectSocket)){
                iResult=-1;
                continue;
            }

            string name,local_name;
            ss>>name>>local_name;
            if(Send(name,DataSocket)){
                iResult=-1;
                continue;
            }
            if(Send(local_name,DataSocket)){
                iResult=-1;
                continue;
            }
            if (isBinary){
                //PutBinary()
             }

        }
        else{
            cout<<"UNKNOWN COMMAND";
        }
    }while (iResult >= 0);
    return 0;
}

