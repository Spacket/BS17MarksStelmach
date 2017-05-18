#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>

int main() {
    int sock, fd, rec_value;
    struct sockaddr_in server;
    char in[2000];
    char out[2000];

    //socket anlegen
    //socket(domain, type, protocol)
    //AF_INET = ARPA Internet Protocols
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("creating stream socket");
        exit(2);
    }

    //socket struct füllen
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY; //jeder hat Zugriff auf den Server
    server.sin_port = htons(4711); //htons: converts an unsigned short integer from host byte order to Internet network byte order

    //socket binden
    //bind(int s, const struct sockaddr *name, int name_length)
    bind(sock, (struct sockaddr *) &server, sizeof(server));

    //auf Verbindung hören
    listen(sock, 5);

    //Verbindung akzeptieren und auslesen von fd
    while (true) {
        fd = accept(sock, 0, 0);
        while (read(fd, in, 2000) > 0) {
            write(fd, out, 2000);
        }

        printf("Ending connection.\n");
        close(fd);
    }

}
