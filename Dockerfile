FROM debian
RUN apt update && \
    apt install git -y && \
    apt install clang -y && \
    git clone https://github.com/zenonet/cppHttpServer.git && \
    mv cppHttpServer/main.c main.c && \
    clang main.c -o main && \
    apt clean
EXPOSE 8080
CMD ["./main"]
