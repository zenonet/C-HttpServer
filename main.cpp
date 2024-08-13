#include <cstring> 
#include <iostream> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <unistd.h>

int main(){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int success = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

    listen(serverSocket, 5);
    std::cout << "Now listening...\n";
    bool quit = false;
    while(!quit){
        int clientSocket = accept(serverSocket, nullptr, nullptr);
    
        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        // std::cout << "Message from client: " << buffer << '\n';

        // Parse header
        int methodLength = 0;
        while(methodLength < sizeof(buffer) && buffer[methodLength] != ' ') methodLength++;
        char* method = (char*) malloc(methodLength+1);
        memcpy(method, &buffer, methodLength);
        method[methodLength] = 0;
        std::cout << "HTTP Method: " << method << '\n';

        int uriLength = 0;
        while(uriLength + methodLength + 1 < sizeof(buffer) && buffer[uriLength+methodLength+1] != ' ') uriLength++;
        char* uri = (char*) malloc(uriLength);
        memcpy(uri, &buffer[methodLength+1], uriLength+1);
        uri[uriLength] = 0;
        std::cout << "Path: " << uri << '\n';
        quit = !strcmp(uri, "/close"); 
        char* response = "HTTP/1.0 200 OK\n\nHello World!"; 
        send(clientSocket, response, strlen(response), 0);

        close(clientSocket);
    }
}
