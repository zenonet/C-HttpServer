 
// #include <iostream> 
// #include <cstdio>
#include <netinet/in.h> 
#include <stdio.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h>
#include <stdlib.h>

enum HttpMethod : char{
    GET,
    POST,
    PUT,
    DELETE
};


struct HttpRequest{
    enum HttpMethod method;
    char* uri;
    int clientSocket;
    char isClosed;
};

struct HttpResponse{
    int statusCode;
    char* body;
};

typedef struct HttpResponse* (*httpRequestHandler)(struct HttpRequest*);

char* getResponse(int code, char* body){
   if(code < 0 || code > 999) return NULL;
   int bodyLength = strlen(body)-1;
   char statusLine[] = "HTTP/1.0 XXX OK\n\n";
   int statusLineLength = sizeof(statusLine)-1;
   sprintf(&statusLine[9], "%d", code);
   statusLine[12] = ' '; // Because sprintf adds a null character after the number, we need to make that a space again
   char* response = malloc(statusLineLength+bodyLength+1);

   memcpy(response, statusLine, statusLineLength);
   memcpy(&response[statusLineLength], body, bodyLength);
   response[statusLineLength+bodyLength] = 0;
   printf("Response length: %lu\n", strlen(response));
   return response;
}

void sendResponse(struct HttpRequest* request, struct HttpResponse* response){
    char* responseString = getResponse(response->statusCode, response->body);
    send(request->clientSocket, responseString, strlen(responseString), 0);
    free(responseString);
    close(request->clientSocket);
    request->isClosed = 1;
}

int main(){
    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(8080);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int success = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    printf("Return value of bind call is %d\n", success);
    listen(serverSocket, 5);
    printf("Now listening...\n");
    char quit = 0;
    while(!quit){
        int clientSocket = accept(serverSocket, 0, 0);
    
        char buffer[1024] = {0};
        recv(clientSocket, buffer, sizeof(buffer), 0);
        // std::cout << "Message from client: " << buffer << '\n';

        // Parse header
        int methodLength = 0;
        while(methodLength < sizeof(buffer) && buffer[methodLength] != ' ') methodLength++;
        char* method = (char*) malloc(methodLength+1);
        memcpy(method, &buffer, methodLength);
        method[methodLength] = 0;

        int uriLength = 0;
        while(uriLength + methodLength + 1 < sizeof(buffer) && buffer[uriLength+methodLength+1] != ' ') uriLength++; 
        char* uri = (char*) malloc(uriLength);
        memcpy(uri, &buffer[methodLength+1], uriLength+1);
        uri[uriLength] = 0;

        struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
        request->isClosed = 0;
        request->clientSocket = clientSocket;
        request->uri = uri;

        // translate method string to enum value
        if(strcmp(method, "GET")) request->method = GET;
        if(strcmp(method, "POST")) request->method = POST;
        if(strcmp(method, "PUT")) request->method = PUT;
        if(strcmp(method, "DELETE")) request->method = DELETE;
       

    
        quit = !strcmp(request->uri, "/close"); 

        //char response[] = "HTTP/1.0 200 OK\n\nHello World!";
        char* response = getResponse(200, "Dazu noch ein bisschen frisches Asbest!");
        printf("Sending: '%s'\n", response);
        send(clientSocket, response, strlen(response), 0);
        free(response);
        close(clientSocket);
    }
}
