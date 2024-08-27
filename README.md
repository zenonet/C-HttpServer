# C HTTP Server

This is a simple HTTP server implementation written in pure C as a challenge. I created this server to learn about C and Socket Programming (and NeoVim).

![1000018603](https://github.com/user-attachments/assets/31011b26-4a1b-452e-87df-c7f42db61bee)


## Why is this called CppHttpServer even though it's in pure C?

When I started this project, I wanted to do it in C++ but I quickly realized that C might be a fun extra challenge so at some point, I changed the code I had so far to be C compliant. Now, it's 100% pure C.

## How it works

The programs contains structs for Http Requests, Responses and Headers and a function pointer
variable with a request handler that receives aǹ `HttpRequest` and must return an `HttpResponse`.

When an HTTP Request is received, the server parses it and puts it in an `HttpRequest` struct. This struct is then passed to a function to handle the request.
This function generates an `HttpResponse` struct that is then serialized by the server and sent back to the client.

## Tools I used

Because I currently don't have access to my main PC, I decided to try to create something on my Android Tablet (Samsung Galaxy Tab S9 with Keyboard Cover).
Obviously, an android device has limited capabilities regarding software development but I was able to get a C/CPP development environment running using Termux, Clang and Neovim. So, I even learned a bit about NeoVim in addition to improving my C/C++ skills.

## How to run it

The server is a single C file that can be compiled using clang. (Probably using gcc as well but I haven't tested that). I think some of the code is also only compatible with linux but who wants to run an HTTP server on some other OS anyway?

To run the server yourself:

- Clone this Repository: `git clone https://github.com/zenonet/CppHttpServer.git`
- Navigate into its directory: `cd CppHttpServer`
- Compile using clang: `clang main.c -o main`
- Run the compiled server: `./main`

## How to run it in docker

So, I could provide the docker image but I am too lazy to do so, so you gotta build it yourself like this:

- Clone this Repository: `git clone https://github.com/zenonet/CppHttpServer.git`
- Navigate into its directory: `cd CppHttpServer`
- Build the Docker image: `docker build -t zenonet/chttp .`
- Create a container with the just built image: `docker run --name cHttpServer -p 8080:8080 -t zenonet/chttp`

Now, you sould be able to access the server at localhost:8080
