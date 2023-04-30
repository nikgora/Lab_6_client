#include <cstdio>
#include <ws2tcpip.h>
#include <string>
#include <vector>
#include <iostream>
#include <cstring>
#include <sstream>
#include <dirent.h>
#include <sys/stat.h>
#include "winsock2.h"
#include "sha-1.h"

#define DEFAULT_BUF_LEN 1024

using namespace std;

void clenup(char *recvbuf, int len) {
    for (int i = len; i < strlen(recvbuf); i++) {
        recvbuf[i] = '\000';
    }
}

struct addrinfo *result = nullptr,
        *ptr = nullptr,
        hints;

bool Recive(string &res, SOCKET DataSocket) {
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
        cout << "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;

    }
    clenup(buffer, len);
    res = buffer;
    return false;
}

bool Send(const string &string, SOCKET DataSocket) {
    int iResult = send(DataSocket, to_string(string.length()).c_str(), DEFAULT_BUF_LEN, 0);
    if (iResult < 0) {
        cout << "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;
    }
    iResult = send(DataSocket, string.c_str(), string.length(), 0);
    if (iResult < 0) {
        cout << "recv failed:\n" << WSAGetLastError();
        closesocket(DataSocket);
        return true;
    }
    return false;
}

bool BindSocket(SOCKET &DataSocket, const string &port, char *argv[]) {
    int iResult;
    bool q = false;
    ZeroMemory(&hints, sizeof(hints));
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
    ptr = result;
// Create a SOCKET for connecting to server
    DataSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
    if (DataSocket == INVALID_SOCKET) {
        printf("Error at socket(): %d\n", WSAGetLastError());
        freeaddrinfo(result);
        WSACleanup();
        return true;
    }
    // Connect to server.
    iResult = connect(DataSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
    if (iResult == SOCKET_ERROR) {
        q = true;
        DataSocket = INVALID_SOCKET;
    }
    // Should really try the next address returned by getaddrinfo
// if the connect call failed

    if (q) {
        ptr = ptr->ai_next;
        DataSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
        iResult = connect(DataSocket, ptr->ai_addr, (int) ptr->ai_addrlen);
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
void makeVector(string s,vector<string> &strings){
    stringstream ss(s);
    string word;
    while (ss >> word) { // Extract word from the stream.
       strings.push_back(word);
    }
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
    if (Recive(res, socket)) return true;
    outputFile.write(res.c_str(), res.length());
    outputFile.close();
    return false;
}

bool Get(SOCKET socket, string name, const string &second_name, string &error) {
    if (!second_name.empty())name = second_name;
    ofstream outputFile;
    string res;
    if (Recive(res, socket))return true;
    outputFile.open(name);
    if (!outputFile) {
        error = "Error opening file.";
        return true;
    }

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

    inputFile.seekg(0, ios::end);
    int len = inputFile.tellg();
    inputFile.seekg(0, ios::beg);
    char l[len];
    inputFile.read(l, len);
    clenup(l,len);
    if (Send(l, socket))return true;
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
    inputFile.close();
    if (Send(l, socket))return true;

    return false;
}

bool endWith(const string &string1, const string &string2) {
    for (int i = 0; i < string2.length(); ++i) {
        if (string1[string1.length() - string2.length() + i + 1] != string2[i])return false;
    }
    return true;
}


bool Dir(string dir_path, string &filter, vector<string> &files, string &error) {

    if (filter.empty())
        filter = "*";
    DIR *dir;
    struct dirent *ent;
    struct stat st;
    char file_path[PATH_MAX];
    if (dir_path[dir_path.length() - 1] != '/') {
        error = "You don't enter a directory";
        return true;
    }
    //snprintf(file_path, PATH_MAX, "%s%s", dir_path.c_str(), ent->d_name);
    dir = opendir(dir_path.c_str());
    if (dir == NULL) {
        error = "Error opening directory";
        return true;
    }
    for (int i = 0; i < filter.length() - 1; ++i) {
        filter[i] = filter[i + 1];
    }
    filter[filter.length() - 1] = '\0';
    // loop through all files in the directory
    while ((ent = readdir(dir)) != NULL) {
        // construct the full path to the file
        char file_path[PATH_MAX];
        snprintf(file_path, PATH_MAX, "%s%s", dir_path.c_str(), ent->d_name);
        // get the file status
        if (stat(file_path, &st) == -1) {
            continue;
        }
        if (S_ISREG(st.st_mode) && endWith(file_path, filter)) {
            string temp = file_path;
            files.push_back(temp);
        }
    }
    return false;
}
bool GetHelps(vector<pair<string, string>> &helps, string &error) {//read all registrated user from file
    ifstream inputFile;
    inputFile.open("../help.txt");
    if (!inputFile) {
        error = "Error opening file.";
        return true;
    }
    string line;
    while (getline(inputFile, line)) {
        // create a stringstream from the line
        // read the first and second words from the stringstream
        string word1, word2;
        stringstream ss(line);
        getline(ss,word1,'~');
        getline(ss,word2,'~');
        pair<string, string> temp;
        temp.first = word1;
        temp.second = word2;

        helps.push_back(temp);
    }
    inputFile.close();
    return false;
}

int main(int argc, char *argv[]) {
    WSADATA wsaData;
    char recvbuf[DEFAULT_BUF_LEN]{};
    int iResult = 0, iSendResult;
    SHA1 nikname, password;
    int recvbuflen = DEFAULT_BUF_LEN;
    int q = 0;
// Initialize Winsock

    iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (iResult != 0) {
        printf("WSAStartup failed: %d\n", iResult);
        return 1;
    }
    // Resolve the server address and port

    auto ConnectSocket = INVALID_SOCKET;
    bool err = BindSocket(ConnectSocket, "12", argv);
    if (err) {
        return 1;
    }
    string command;
    printf("Connect to server is successful!\n");
    string directory = "./";
    bool isBinary = false;
    int anonymusCode = 0;//0-Not login, 1-anonym mode, 2- user mode
    bool isOpen = false;
    auto DataSocket = INVALID_SOCKET;
    bool isPrompt = false;
    do {
        cout << "\nEnter command:\n";
        string line;
        // create a stringstream from the line
        getline(cin, line);
        stringstream ss(line);
        ss >> command;
        if (command == "open") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            if (BindSocket(DataSocket, "13", argv)) {
                iResult = -1;
                continue;
            }
            isOpen = 1;
        }
        else if(command=="help"){
            vector<pair<string, string>> helps;
            string  com;
            ss>>com;
            string error;
            if (GetHelps(helps,error))
            {iResult=-1;
            cout<<error;
                continue;}
            for (auto item : helps) {
                bool q = false;
                string itfirst=item.first;
                if (com.empty()){
                    q= true;
                    com= item.first;
                }
                if (itfirst==com){
                    cout<<item.first<<" - "<<item.second<<endl;
                }
                if(q){
                    com="";
                }
            }
        }
        else if (command == "lcd") {
            ss >> directory;
        }
        else if (command == "quit") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            if(isOpen)closesocket(DataSocket);
            iResult = -1;
            isOpen = false;
            isBinary = false;
            anonymusCode = 0;

        }
        else if (!isOpen) {
            cout << "Socket must be open\n";
            continue;
        }
        else if (command == "login") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            string nik;
            ss >> nik;
            nikname.update(nik);
            bool anonym = false;
            if (nik == "") {
                password.update("");
                anonym = true;
            }
            else {
                cout << "password: ";
                string pass;
                cin >> pass;
                password.update(pass);
                getline(cin, line);
            }
            if (Send(nikname.final(), DataSocket)) {
                iResult = -1;
                continue;
            }
            if (Send(password.final(), DataSocket)) {
                iResult = -1;
                continue;
            }
            string res;
            if (Recive(res, DataSocket)) {
                iResult = -1;
                continue;
            }
            if (res == "Login successful") {

                if (anonym) {
                    anonymusCode = 1;
                }
                else {
                    anonymusCode = 2;
                }
            }
            else {
                anonymusCode = 0;
            }
            cout << res;

        }
        else if (command == "close") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            isOpen = false;
            isBinary = false;
            anonymusCode = 0;
            directory="./";
            isPrompt=false;
            closesocket(DataSocket);
        }
        else if (!anonymusCode) {
            cout << "You must be at least anonym";
            continue;
        }
        else if (command=="prompt"){
            isPrompt= true;
        }
        else if (command == "cd") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            // read the first and second words from the stringstream

            string newDir;
            ss >> newDir;
            if (Send(newDir, DataSocket)) {
                iResult = -1;
                continue;
            }
        }
        else if (command == "dir") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            string file;
            ss >> file;
            if (file == "")file = "*";
            if (Send(file, DataSocket)) {
                iResult = -1;
                continue;
            }
            string res;
            if (Recive(res, DataSocket)) {
                iResult = -1;
                continue;
            }
            cout << res;
        }

        else if (command == "mget") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            string name;
            ss >> name;
            vector<string> files;
            if (name == "")name = "*";
            if (Send(name, DataSocket)) {
                iResult = -1;
                continue;
            }
            string res;
            if (Recive(res, DataSocket)) {
                iResult = -1;
                continue;
            }
            makeVector(res,files);
            for(auto file: files){
                stringstream sss(file);
                string word1;
                while (getline(sss, word1,'/')) {
                }
                file = word1;
                bool isSave = false;
                if (!isPrompt){
                    cout<<"Get file "<<file<<" from server\nY/y/yes/Yes for save, other for no save: ";
                    string s;
                    cin>>s;
                    if (s=="Y"||s=="y"||s=="yes"||s=="Yes") isSave= true;
                    getline(cin, line);
                }
                else{
                    isSave= true;
                }
                string error;
                if (!isSave) continue;
                if(Send("cont",DataSocket)){
                    iResult=-1;
                    break;
                }
                if (Send(file, DataSocket)) {
                    iResult = -1;
                    break;
                }
                if (isBinary) {
                    if (GetBinary(DataSocket, file, "", error)) {
                        iResult = -1;
                        cout << error;
                        break;
                    }
                }
                else {
                    if (Get(DataSocket, file, "", error)) {
                        iResult = -1;
                        cout << error;
                        break;
                    }

                }
            }
            if (Send( "end",DataSocket)) {
                iResult = -1;
                continue;
            }
        }

        else if (command == "get") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            string name, local_name;
            string error;
            ss >> name >> local_name;
            if (Send(name, DataSocket)) {
                iResult = -1;
                continue;
            }
            if (isBinary) {
                if (GetBinary(DataSocket, name, local_name, error)) {
                    iResult = -1;
                    cout << error;
                    continue;
                }
            }
            else {
                if (Get(DataSocket, name, local_name, error)) {
                    iResult = -1;
                    cout << error;
                    continue;
                }

            }

        }
        else if (command == "ascii") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            isBinary = false;
        }
        else if (command == "binary") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            isBinary = true;
        }
        else if (command == "user") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            string nik;
            ss >> nik;
            nikname.update(nik);
            bool anonym = false;
            if (nik == "") {
                password.update("");
                anonym = true;
            }
            else {
                cout << "password: ";
                string pass;
                cin >> pass;
                password.update(pass);
                getline(cin, line);
            }
            if (Send(nikname.final(), DataSocket)) {
                iResult = -1;
                continue;
            }
            if (Send(password.final(), DataSocket)) {
                iResult = -1;
                continue;
            }
            string res;
            if (Recive(res, DataSocket)) {
                iResult = -1;
                continue;
            }
            if (res == "Login successful") {

                if (anonym) {
                    anonymusCode = 1;
                }
                else {
                    anonymusCode = 2;
                }
            }
            else {
                anonymusCode = 0;
            }
            cout << res;
        }
        else if (command == "pwd") {
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            string res;
            if (Recive(res, DataSocket)) {
                iResult = -1;
                continue;
            }
            cout << res;
        }
            /*
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

        else if (anonymusCode < 2) {
            cout << "You must be login as user\n";
            continue;
        }
        else if (command == "put") {
            string error;
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            string name, local_name;
            ss >> name >> local_name;
            if (Send(name, DataSocket)) {
                iResult = -1;
                continue;
            }
            if (Send(local_name, DataSocket)) {
                iResult = -1;
                continue;
            }
            if (isBinary) {
                if (PutBinary(DataSocket, name, error)) {
                    cout << error;
                    iResult = -1;
                    continue;
                }
            }
            else {
                if (Put(DataSocket, name, error)) {
                    cout << error;
                    iResult = -1;
                    continue;
                }

            }

        }
        else if (command == "mput") {
            string error;
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }

            string filter;
            ss >> filter;
            vector<string> files;
            if (
            Dir(directory,filter,files,error)){
                cout<<error;
                iResult=-1;
                continue;
            }
            for(auto file: files){
                stringstream sss(file);
                string word1;
                while (getline(sss, word1,'/')) {
                }
                file = word1;
                bool isSave = false;
                if (!isPrompt){
                    cout<<"Put file "<<file<<" to server\nY/y/yes/Yes for save, other for no save: ";
                    string s;
                    cin>>s;
                    if (s=="Y"||s=="y"||s=="yes"||s=="Yes") isSave= true;
                    getline(cin, line);
                }
                else{
                    isSave= true;
                }
                string error;
                if (!isSave) continue;
                if(Send("cont",DataSocket)){
                    iResult=-1;
                    break;
                }
                if (Send(file, DataSocket)) {
                    iResult = -1;
                    break;
                }
                if (isBinary) {
                    if (PutBinary(DataSocket, file, error)) {
                        iResult = -1;
                        cout << error;
                        break;
                    }
                }
                else {
                    if (Put(DataSocket, file, error)) {
                        iResult = -1;
                        cout << error;
                        break;
                    }
                }
            }
            if (Send( "end",DataSocket)) {
                iResult = -1;
                continue;
            }
        }
        else if (command=="system"){
            if (Send(command, ConnectSocket)) {
                iResult = -1;
                continue;
            }
            string res;
            if(Recive(res,DataSocket)){
                iResult=-1;
                continue;
            }
            cout<<res;
        }
        else {
            cout << "UNKNOWN COMMAND";
        }

    } while (iResult >= 0);
    closesocket(ConnectSocket);
    return 0;
}

