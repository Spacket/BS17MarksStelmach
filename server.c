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
#include <sys/msg.h>
#include <sys/ipc.h>

void bzero (void *to, size_t count){
    memset (to, 0, count);
}

int main(int argc, char *argv[]) {
    int sock, fd, read_size, id, sem_id, *rc, id2, id3;
    socklen_t client_len;
    struct sockaddr_in server, client;
    char in[2000], out[2000];
    char res[50];
    char *temp = &res; //pointer auf res string

    int sub;
    int *temp2 = &sub;

    char del[] = " ";
    data *sm;
    char *tok[3];
    int pid;
    int subs [5];
    int count = 0;
    int v;
    message m;

    sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("creating stream socket");
        exit(2);
    }

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(4993); //Convert multi-byte integer types from host byte order to network byte order

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

    //message queues
    id3 = msgget(IPC_PRIVATE ,IPC_CREAT | 0777 );


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

                    if(*rc == 0){
                        semop(sem_id, &down_w, 1);
                        printf("\nKritischen Bereich betreten\n");
                        PUT(tok[1], tok[2], temp, sm, subs, temp2); // Referenz auf erste element

                        if(sub != 10){
                            m.mtype = sub + 1;
                            char str[100];
                            snprintf(str, sizeof(str), "Key: %s Neue Value: %s ", sm[sub].key, sm[sub].value);
                            strcpy(m.mtext, str);

                            if (msgsnd(id3, &m, 32, 0) < 0) {
                                perror("msgsnd failed");
                                exit(1);
                            }
                        }

                        semop(sem_id, &up_w, 1);
                    }else{
                        printf("%d Leser aktiv - Zugriff verweigert !\n", *rc);
                    }
                }else if(strcmp(tok[0], "get") == 0){
                    semop(sem_id, &down_r, 1); //erhalte Zugriff auf rc
                    *rc += 1;
                    if(*rc == 1) {                   //erste Leser verweigert Zugriff für Schreibprozesse
                        semop(sem_id, &down_w, 1);
                        printf("Erster Leser - Zugang erhalten\n");
                    }
                    semop(sem_id, &up_r, 1);  //Zugriff auf rc freigeben
                    printf("Leser hinzugefügt - Anzahl: %d\n", *rc);
                    GET(tok[1], temp, sm);

                    semop(sem_id, &down_r, 1);  //erhalte Zugriff auf rc
                    *rc -= 1;
                    if(*rc == 0) {                   //letzte Leser erlaubt Zugriff für Schreibprozesse
                        semop(sem_id, &up_w, 1);
                        printf("\nLetzter Leser - Freigabe für Schreibprozesse\n");
                    }
                    semop(sem_id, &up_r, 1);        //Zugriff auf rc freigeben

                }else if(strcmp(tok[0], "del") == 0) {
                    semop(sem_id, &down_w, 1);
                    DEL(tok[1], sm);
                    semop(sem_id, &up_w, 1);

                }else if(strcmp(tok[0], "sub") == 0){
                    int a = 0;
                    int i;

                    for(i = 0; i < 32; i++){
                        if(strcmp(sm[i].key, tok[1]) == 0){
                            subs[count] = i;

                            char str2[100];
                            snprintf(str2, sizeof(str2), "Key %s abonniert !\n", sm[i].key);
                            strcpy(out, str2); //res in out
                            write(fd, out, strlen(out)); //out ausgeben
                            bzero(out, sizeof(out)); //out auf 0 setzen


                            a = 1;
                            count++;
                        }

                    }

                    if(a == 0){
                        printf("\nSchlüssel nicht gefunden.\n", tok[1]);
                    }

                }else if(strcmp(tok[0], "sync") == 0){

                    /*int x;
                    for(x = 0; x < count; x++){
                            printf("\n%d Abo mit Schlüssel %s\n", x+1,sm[subs[x]].key);
                    }*/

                    if(msgrcv(id3, &m, 32, 1, IPC_NOWAIT) < 0){
                        perror("msgrcv failed");
                        exit(1);
                    }else{

                        strcpy(out, m.mtext); //res in out
                        write(fd, out, strlen(out)); //out ausgeben
                        bzero(out, sizeof(out)); //out auf 0 setzen
                    }

                }else if(strcmp(tok[0], "close") == 0){
                    semctl(sem_id, 0, IPC_RMID);
                    semctl(sem_id, 1, IPC_RMID);
                    shmctl(id, IPC_RMID, 0);
                    shmctl(id2, IPC_RMID, 0);
                    msgctl(id3,IPC_RMID,NULL);
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
