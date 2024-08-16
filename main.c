 
#include <netinet/in.h> 
#include <stdio.h>
#include <string.h>
#include <sys/socket.h> 
#include <unistd.h>
#include <stdlib.h>

#define PORT 8080

typedef enum HttpMethod : char{
    GET,
    POST,
    PUT,
    DELETE
} HttpMethod;

typedef struct HttpHeader{
    char* headerName;
    char* value;
} HttpHeader;

typedef struct HttpRequest{
    enum HttpMethod method;
    char* uri;
    HttpHeader* headers;
    int headerCount;

    int clientSocket;
    char isClosed;
} HttpRequest;

typedef struct HttpResponse{
    int statusCode;
    char* body;
} HttpResponse;

typedef HttpResponse* (*httpRequestHandler)(HttpRequest*);

int serverSocket;

char* getResponse(int code, char* body){
   if(code < 0 || code > 999) return NULL;
   int bodyLength = strlen(body);
   char statusLine[] = "HTTP/1.0 XXX OK\nContent-Type: text/html\n\n";
   int statusLineLength = sizeof(statusLine)-1;
   sprintf(&statusLine[9], "%d", code);
   statusLine[12] = ' '; // Because sprintf adds a null character after the number, we need to make that a space again
   char* response = malloc(statusLineLength+bodyLength+1);

   // Copy the status line to the response string
   memcpy(response, statusLine, statusLineLength);

   // Copy the response body to the response string
   memcpy(&response[statusLineLength], body, bodyLength);
   response[statusLineLength+bodyLength] = 0;
   printf("Response length: %lu\n", strlen(response));
   return response;
}

void sendResponse(struct HttpRequest* request, struct HttpResponse* response){
    char* responseString = getResponse(response->statusCode, response->body);
    send(request->clientSocket, responseString, strlen(responseString), 0);
    printf("Sent response\n");
    free(responseString);
}


HttpResponse* myHandler(HttpRequest* request){

    if(!strcmp(request->uri, "/close")){
        printf("Terminating because /close endpoint was called..\n");
        close(request->clientSocket);
        close(serverSocket);
        exit(0);
    }
    printf("Receved request for '%s'\n", request->uri);

    for(int i=0;i<request->headerCount;i++){
        //printf("Got header '%s' with value '%s'\n", request->headers[i].headerName, request->headers[i].value);
    }
    HttpResponse* response = malloc(sizeof(HttpResponse));
    response->body = "Hello Hackclub! <br><a href=\"/close\">Close server</a>";
    response->statusCode = 200;
    return response;
}

httpRequestHandler globalHandler;
int main(){

    globalHandler = &myHandler;  

    serverSocket = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in serverAddress;
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    int problem = bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));
    printf("Return value of bind call is %d\n", problem);

    if(problem){
        printf("Unable to bind socket for port %d. Terminating...\n", PORT);
        return 1;
    }

    listen(serverSocket, 5);
    printf("Now listening...\n");
    char quit = 0;
    while(!quit){
        int clientSocket = accept(serverSocket, 0, 0);
    
        char buffer[1024] = {0};
        int length = recv(clientSocket, buffer, sizeof(buffer), 0);
        printf("Request size: %d\n", length);

        // Parse status line :
        int methodLength = 0;
        while(methodLength < sizeof(buffer) && buffer[methodLength] != ' ') methodLength++;
        char* method = (char*) malloc(methodLength+1);
        memcpy(method, &buffer, methodLength);
        method[methodLength] = 0;

        // Parse URI:
        int uriLength = 0;
        while(uriLength + methodLength + 1 < sizeof(buffer) && buffer[uriLength+methodLength+1] != ' ') uriLength++; 
        char* uri = (char*) malloc(uriLength);
        memcpy(uri, &buffer[methodLength+1], uriLength+1);
        uri[uriLength] = 0;

        // Parse request headers:
        int headerCount = 0;
        int i = methodLength+1+uriLength+1;
        for(;i < length;i++){ // Count how many request headers there are
            if(buffer[i] != '\n') continue; 
            if(buffer[i+1] == '\n') break;
            headerCount++;
        }
        printf("--------------------------------------------------------Header count: %d--------------------------------------------\n", headerCount);

        int endOfHeaders = i;

        HttpHeader* headers = malloc(sizeof(HttpHeader)*headerCount); // Allocate accordingly
        i = methodLength+1+uriLength+2;
        int startOfLine = i;
        int currentHeader = 0;
        int colonIndex = -1; // Colon index relative to the start of the line
        for(; i<endOfHeaders; i++){
            if(buffer[i] == '\n'){ // If header line is complete
                char* header = malloc(i-startOfLine); // Yes, this means there is also space for the line break but we need that for a null terminator anyway
                memcpy(header, &buffer[startOfLine], i-1-startOfLine);
                header[colonIndex] = 0; // Make the colon a null char to terminate the header name
                header[i-startOfLine-1] = 0; // Add null terminator to the header value

                char* value = header + colonIndex + 1; // Get a pointer to the value
                printf("Set second null terminator at %d. Colon is at %d\n", i-startOfLine-1, colonIndex);
                // exit(0);
                headers[currentHeader].headerName = header;
                headers[currentHeader++].value = value;
                printf("Value: %s\n", &header[colonIndex+1]);
                colonIndex = -1;
            }
            if(buffer[i] == ':') colonIndex = i - startOfLine;
        }


        struct HttpRequest* request = (struct HttpRequest*)malloc(sizeof(struct HttpRequest));
        request->isClosed = 0;
        request->clientSocket = clientSocket;
        request->uri = uri;
        request->headers = headers;
        request->headerCount = headerCount;

        // translate method string to enum value
        if(strcmp(method, "GET")) request->method = GET;
        if(strcmp(method, "POST")) request->method = POST;
        if(strcmp(method, "PUT")) request->method = PUT;
        if(strcmp(method, "DELETE")) request->method = DELETE;
       

        HttpResponse* response = globalHandler(request);

        sendResponse(request, response);
        close(request->clientSocket);
        request->isClosed = 1;

        // quit = !strcmp(request->uri, "/close"); 

        //char response[] = "HTTP/1.0 200 OK\n\nHello World!";
        //char* response = getResponse(200, "Dazu noch ein bisschen frisches Asbest!");
        //printf("Sending: '%s'\n", response);
        //send(clientSocket, response, strlen(response), 0);
        // free(response->body);
        free(response);
        // close(clientSocket);
    }
}
