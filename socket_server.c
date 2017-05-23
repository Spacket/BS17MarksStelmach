#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>

int main() {
    int sock, fd, client_len;
    struct sockaddr_in server, client;
    char in[2000];
    char out[2000];

    client_len = sizeof(client);

    //Socket anlegen
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("creating stream socket");
        exit(2);
    }

    //Socket struct füllen
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = hton1(INADDR_ANY); //jeder hat Zugriff auf den Server
    server.sin_port = htons(4711); //htons: converts an unsigned short integer from host byte order to Internet network byte order

    //Socket binden
    if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("bind socket to server_addr");
        exit(1);
    }

    //auf Verbindungen warten
    if (listen(sock, 5) < 0) {
        perror("listening for connections");
    }

    //Verbindung akzeptieren und Auslesen von fd
    while (true) {
        fd = accept(sock, &client, &client_len);

        while (read(fd, in, 2000) > 0) {
            write(fd, out, 2000);
        }
        close(fd);
    }
}


