#include <ctype.h>
// #include <linux/time.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
// #include <sys/time.h>
// #include <time.h>
#include <unistd.h>

#define PORT 8080

typedef enum HttpMethod : char { GET, POST, PUT, DELETE } HttpMethod;

typedef struct HttpHeader {
	char *headerName;
    char *value;
} HttpHeader;

typedef struct HttpRequest {
	enum HttpMethod method;
	char *uri;
	HttpHeader *headers;
	int headerCount;

	int clientSocket;
	char isClosed;
} HttpRequest;

typedef struct HttpResponse {
	int statusCode;
	char *body;
	HttpHeader *headers;
	int headerCount;
} HttpResponse;

typedef HttpResponse *(*httpRequestHandler)(HttpRequest *);


long swStart(){
    struct timespec spec;
    clock_gettime(CLOCK_REALTIME, &spec);
    return spec.tv_nsec;
}
double swStop(long start){
    return ((double)swStart() - (double)start)/1000;
}

int serverSocket;
long requestSw = -1;

char *getResponseString(HttpResponse *response, int* respLenPtr) {
    long serialierSw = swStart();

	if (response->statusCode < 0 || response->statusCode > 999)
		return NULL;

    long statusSw = swStart();
	int bodyLength = strlen(response->body);
	char statusLine[] = "HTTP/1.0 XXX OK\n";
	int statusLineLength = sizeof(statusLine) - 1;
	sprintf(&statusLine[9], "%d", response->statusCode);
	statusLine[12] = ' '; // Because sprintf adds a null character after the
						  // number, we need to make that a space again

	// Calculate space needed for header section
	int headerSectionLength = 0;
	for (int i = 0; i < response->headerCount; i++) { // Foreach header
		headerSectionLength += strlen(response->headers[i].headerName) + 1 +
							   strlen(response->headers[i].value + 1);
	}

    printf("    Generating status line and calculating needed space took %fms\n", swStop(statusSw));
    
    *respLenPtr = statusLineLength + headerSectionLength + 1 + bodyLength + 1;
	char *responseString =
		malloc(*respLenPtr);

	// Copy the status line to the response string
	memcpy(responseString, statusLine, statusLineLength);

    long headerSw = swStart();
	// Serialize headers
	int pos = statusLineLength;
	for (int i = 0; i < response->headerCount; i++) {
		int nameLength = strlen(response->headers[i].headerName);
		int valueLength = strlen(response->headers[i].value);

		memcpy(responseString + pos, response->headers[i].headerName,
			   nameLength);
		pos += nameLength;

		responseString[pos++] = ':';

		memcpy(responseString + pos, response->headers[i].value, valueLength);
		pos += valueLength;
		responseString[pos++] = '\n';
	}
	responseString[pos++] = '\n';
    printf("    Serializing headers took %fms\n", swStop(headerSw));

	// Copy the response body to the response string
	memcpy(responseString + pos, response->body, bodyLength);
	pos += bodyLength;
	responseString[pos++] = 0;
	// printf("Response length: %lu\n", strlen(responseString));
    printf("Response serialization took %fms in total\n", swStop(serialierSw));
	return responseString;
}

void sendResponse(struct HttpRequest *request, struct HttpResponse *response) {
    int respLen;
	char *responseString = getResponseString(response, &respLen);
    long sendSw = swStart();
	send(request->clientSocket, responseString, respLen, 0);
	free(responseString);
    printf("Sent response in %fms\n", swStop(sendSw));
}

HttpResponse *myHandler(HttpRequest *request) {

	printf("Receved request for '%s'\n", request->uri);
    char* body;
	if (!strcmp(request->uri, "/amogus")) {
		body = "Is it just me or is this endpoint kinda sus...";
	}else{
        char bodyTemplate[] = "<body style=\"font-family:sans-serif;padding:24pt;ddisplay:flex; margin: auto;background:black;color:white;flex-direction:column;\"><h2>Hello Hackclub!</h2><p>I wrote this HTTP server in pure C as a challenge. Additionally this server was completely developed in NeoVim in Termux on an Android Tablet with a physical keyboard. It was kinda fun! The source-code is available <a href=\"https://github.com/zenonet/cppHttpServer\">here</a>.</p><p>Unfortunately, the server is kinda slow (even though it's written in pure C). That's probably because I am a bad programmer but also because writing to the socket stream using the send() function takes forever apparently. (largest write-time I've seen was ~250ms)</p><p>As of writing this to a char*, %d ms have passed since I received your request.</p></body>g";

        bodyTemplate[sizeof bodyTemplate-1] = 0;
        body = malloc(sizeof bodyTemplate+5);
        // memcpy(body, bodyTemplate, sizeof bodyTemplate);

        // Insert time into the message
        int ms = (int)swStop(requestSw);
        if(ms < 1000)
            sprintf(body, bodyTemplate, ms);
        printf("Full body:\n%s\n", body);
    }
    /*
	for (int i = 0; i < request->headerCount; i++) {
		// printf("Got header '%s' with value '%s'\n",
		// request->headers[i].headerName, request->headers[i].value);
	}
    */

	HttpResponse *response = malloc(sizeof(HttpResponse));
	response->body = body;
	response->statusCode = 200;
    #define HEADER_COUNT 2
    response->headers = malloc(sizeof(HttpHeader) * HEADER_COUNT);
    response->headers[0].headerName = "Content-Type";
    response->headers[0].value = "text/html";
    response->headers[1].headerName = "Server";
    response->headers[1].value = "zenonet's pure C HTTP server";
    response->headerCount = HEADER_COUNT;
	return response;
}

httpRequestHandler globalHandler;
int main() {

	globalHandler = &myHandler;

	serverSocket = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in serverAddress;
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;

	int problem = bind(serverSocket, (struct sockaddr *)&serverAddress,
					   sizeof(serverAddress));
	printf("Return value of bind call is %d\n", problem);

	if (problem) {
		printf("Unable to bind socket for port %d. Terminating...\n", PORT);
		return 1;
	}
    
    // int one = 1;
    // setsockopt(descriptor, SOL_TCP, TCP_NODELAY, &one, sizeof(one));

	listen(serverSocket, 5);
	printf("Now listening...\n");
	char quit = 0;
	while (!quit) {
		int clientSocket = accept(serverSocket, 0, 0);

		char buffer[1024] = {0};
		int length = recv(clientSocket, buffer, sizeof(buffer), 0);
		printf("Request size: %d\n", length);
        
        long sw = swStart();
        requestSw = sw;
		// Parse status line :
		int methodLength = 0;
		while (methodLength < sizeof(buffer) && buffer[methodLength] != ' ')
			methodLength++;
		char *method = (char *)malloc(methodLength + 1);
		memcpy(method, &buffer, methodLength);
		method[methodLength] = 0;

		// Parse URI:
		int uriLength = 0;
		while (uriLength + methodLength + 1 < sizeof(buffer) &&
			   buffer[uriLength + methodLength + 1] != ' ')
			uriLength++;
		char *uri = (char *)malloc(uriLength);
		memcpy(uri, &buffer[methodLength + 1], uriLength + 1);
		uri[uriLength] = 0;

		int i = methodLength + 1 + uriLength + 1;
		// Go forward to the next line
		while (buffer[i - 1] != '\n')
			i++;
		// Parse request headers:
		int headerCount = 0;
		for (; i < length; i++) { // Count how many request headers there are
			if (buffer[i] != '\n')
				continue;
			if (buffer[(i) + 1] == '\n')
				break;
			headerCount++;
		}

		int endOfHeaders = i;

		HttpHeader *headers =
			malloc(sizeof(HttpHeader) * headerCount); // Allocate accordingly
		i = methodLength + 1 + uriLength;

		int startOfLine = i;
		int currentHeader = 0;
		int colonIndex = -1; // Colon index relative to the start of the line
		for (; i < endOfHeaders; i++) {
			if (buffer[i] == '\n') { // If header line is complete
				if (startOfLine == i - 1)
					break; // If this is the second line break in a row, break
						   // from the loop

				char *header = malloc(
					i - startOfLine); // Yes, this means there is also space for
									  // the line break but we need that for a
									  // null terminator anyway
				memcpy(header, &buffer[startOfLine], i - 1 - startOfLine);

				header[i - startOfLine - 1] =
					0; // Add null terminator to the header value

				header[colonIndex] = 0; // Make the colon a null char to
										// terminate the header name
				char *value =
					header + colonIndex + 1; // Get a pointer to the value
				while (value < header + (i - startOfLine - 1) &&
					   isspace(*value) && *value != '\n')
					value++; // Skip any whitespace after the semicolon
				headers[currentHeader].headerName = header;
				headers[currentHeader++].value = value;
				colonIndex = -1;

				startOfLine = i + 1;
			}
			if (buffer[i] == ':' && colonIndex == -1)
				colonIndex = i - startOfLine;
		}

		struct HttpRequest *request =
			(struct HttpRequest *)malloc(sizeof(struct HttpRequest));
		request->isClosed = 0;
		request->clientSocket = clientSocket;
		request->uri = uri;
		request->headers = headers;
		request->headerCount = headerCount;

		// translate method string to enum value
		if (strcmp(method, "GET"))
			request->method = GET;
		if (strcmp(method, "POST"))
			request->method = POST;
		if (strcmp(method, "PUT"))
			request->method = PUT;
		if (strcmp(method, "DELETE"))
			request->method = DELETE;
        printf("Parsing took %fms\n", swStop(sw));
		HttpResponse *response = globalHandler(request);

        long sendSw = swStart();
		sendResponse(request, response);
        printf("Serializing and sending took %fms\n", swStop(sendSw));

        printf("Process request took %fms\n", swStop(sw));

		close(request->clientSocket);
		request->isClosed = 1;

        free(request->uri);
        for (int i=0;i < request->headerCount; i++) {
            free(request->headers[i].headerName); // We don't need to free the headers value because it was allocated together with the header name
        }
        free(request->headers);
        free(request);
		free(response->body);

        // Not freeing these will mean non readonly headers will cause memory leaks but who needs those anyway
        /*
        for(int i=0;i<response->headerCount; i++){
            free(response->headers[i].headerName);
            free(response->headers[i].value);
        }
        */
        free(response->headers);
		free(response);
		// close(clientSocket);
	}
}
