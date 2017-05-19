#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>

int main(int argc, char *argv[]) {
    int sock, fd;
    char buffer[2000];
    struct sockaddr_in server;
    struct hostent *host_info;

    //Erzeuge Socket - Verbindung über TCP - Kommunikationsendpunkt
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0){
        perror("open stream socket");
        exit(1);
    }

    server.sin_family = AF_INET;

    host_info = gethostbyname(argv[1]); //IP-Adresse des Servers ermitteln

    if(host_info == NULL){
        fprintf(stderr, "%s unknown host.\n", argv[1]);
        exit(1);
    }

    memcpy(&server.sin_addr, host_info->h_addr, host_info->h_length); //Kopieren der i-adresse in die server adresse

    server.sin_port = htons(4711); //Konvertierung in Network Byte Order - Portnummer, über die wir mit dem Server kommunizieren wollen

    //Verbindung zum Server aufbauen
    if(connect(sock, &server, sizeof(server)) < 0){
        perror("connecting stream socket");
        exit(1);
    }

    //Inhalt auslesen
    while(fd = read(0,buffer,2000)){
        if(fd < 0){
            perror("error reading from stdin");
            exit(1);
        }
        //Inhalt in stream socket
        if(write(sock, buffer, fd) < 0){
            perror("writing on stream socket");
            exit(1);
        }
        close(sock);
    }


}