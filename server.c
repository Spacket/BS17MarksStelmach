#include "data.h"
#include <stdio.h>
#include <sys/socket.h>
#include <errno.h>
#include <arpa/inet.h> //definitions for internet operations
#include <netinet/in.h> //Internet address family
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/wait.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/types.h>




void bzero (void *to, size_t count){
    memset (to, 0, count);
}

int main(int argc, char *argv[]) {
    int sock, fd, read_size, id, sem_id, *rc, id2;
    socklen_t client_len;
    struct sockaddr_in server, client;
    char in[2000], out[2000];
    char res[32];
    char *temp = &res; //pointer auf res string
    char del[] = " ";
    int i, j, y;
    data *sm;
    char *tok[3];
    int pid;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("creating stream socket");
        exit(2);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(4934); //Convert multi-byte integer types from host byte order to network byte order

    if (bind(sock, (struct sockaddr *) &server, sizeof(server)) < 0) {
        perror("bind socket to server_addr");
        exit(1);
    }

    if (listen(sock, 5) < 0) {
        perror("listening for connections");
    }

    client_len = sizeof(client);

    //sm für rc
    id = shmget(IPC_PRIVATE, sizeof(int),IPC_CREAT | 0777);

    if (id < 0){
        printf ("Fehler bei key %d, mit der size %d\n", IPC_PRIVATE, SEGSIZE);
    }

    rc = shmat(id2,NULL,0);
    *rc = 0;

    //sm für Daten
    id2 = shmget(IPC_PRIVATE, sizeof(data),IPC_CREAT | 0777);

    if (id2 < 0){
        printf ("Fehler bei key %d, mit der size %d\n", IPC_PRIVATE, SEGSIZE);
    }

    sm = (data *) shmat(id,NULL,0);

    //Semaphore anlegen
    sem_id = semget(IPC_PRIVATE , 2, IPC_CREAT | 0777);

    if(sem_id == -1){
        perror("Die Gruppe konnte nicht angelegt werden!");
        exit(1);
    }

    unsigned short marker[1];
    marker[0] = 1;
    marker[1] = 1;
    semctl(sem_id, 2, SETALL, marker);

    struct sembuf down_w, up_w, down_r, up_r;
    down_w.sem_num = up_w.sem_num = 0;
    down_w.sem_flg = up_w.sem_flg = SEM_UNDO;
    down_w.sem_op = -1;
    up_w.sem_op = 1;

    down_r.sem_num = up_r.sem_num = 1;
    down_r.sem_flg = up_r.sem_flg = SEM_UNDO;
    down_r.sem_op = -1;
    up_r.sem_op += 1;

    while (1) {
        fd = accept(sock, (struct sockaddr*)&client, &client_len);
        pid = fork();

        if(pid < 0) {
            perror("ERROR on fork");
            exit(1);
        }

        else if(pid > 0) {
            //Vaterprozess
            close(fd);
        }

        else if (pid == 0) {
            close(sock);

            while ((read_size = recv(fd, in, 2000, 0)) > 0) {
                strtoken(in, del, tok, 3);

                printf("\n---------------------------------------\n");

                if(strcmp(tok[0], "put") == 0){
                    printf("PUT gewählt - RC: %d\n", *rc);
                    if(*rc == 0){
                        semop(sem_id, &down_w, 1);
                        PUT(tok[1], tok[2], temp, sm); // Referenz auf erste element
                        semop(sem_id, &up_w, 1);
                    }
                }else if(strcmp(tok[0], "get") == 0){
                    semop(sem_id, &down_r, 1); //erhalte Zugriff auf rc
                    *rc += 1;
                    if(*rc == 1) {                   //erste Leser verweigert Zugriff für Schreibprozesse
                        semop(sem_id, &down_w, 1);
                        printf("Erster Leser - Zugang erhalten\n");
                    }
                    semop(sem_id, &up_r, 1);  //Zugriff auf rc freigeben
                    printf("Leser hinzugefügt - Anzahl %d\n", *rc);
                    sleep(10);
                    GET(tok[1], temp, sm);

                    semop(sem_id, &down_r, 1);  //erhalte Zugriff auf rc
                    *rc -= 1;
                    if(*rc == 0) {                   //letzte Leser erlaubt Zugriff für Schreibprozesse
                        semop(sem_id, &up_w, 1);
                        printf("\nLetzter Leser - Freigabe für Schreibprozesse\n");
                    }
                    semop(sem_id, &up_r, 1);        //Zugriff auf rc freigeben

                }else if(strcmp(tok[0], "del") == 0){
                    semop(sem_id, &down_w, 1);
                    DEL(tok[1], sm);
                    semop(sem_id, &up_w, 1);
                }else if(strcmp(tok[0], "close") == 0){
                    semctl(sem_id, 0, IPC_RMID);
                    semctl(sem_id, 1, IPC_RMID);
                    shmctl(id, IPC_RMID, 0);
                    shmctl(id2, IPC_RMID, 0);
                    shutdown(fd, 2);
                } else{
                    printf("\nFalsche Eingabe !\n");
                }


                bzero(in, sizeof(in)); //in auf 0 setzen
                strcpy(out, res); //res in out
                bzero(res, sizeof(res)); //res auf 0 setzen
                write(fd, out, strlen(out)); //out ausgeben
                bzero(out, sizeof(out)); //out auf 0 setzen

            }

            close(fd);

            exit(0);
        }
        else {
            close(fd);
        }


    }
}