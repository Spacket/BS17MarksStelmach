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

//clears a buffer
void bzero (void *to, size_t count){
    memset (to, 0, count);
}

int main(int argc, char *argv[]) {

    //variables for socket server
    int sock, fd, read_size, id2, pid;
    struct sockaddr_in server, client;
    char in[2000], out[2000];
    char res[50];
    char *temp = &res;
    char del[] = " ";
    char *tok[3];
    socklen_t client_len;
    data *sm;

    //variables for semaphore
    int sem_id, *rc, id;
    unsigned short marker[1];

    //variables for pub/sub
    int sub, pid2;
    int *temp2 = &sub;
    int *subs;
    int id3, id4, id5, id6;
    int count = 0;
    int v;
    message m;
    int *aboCount;
    int *subsCount;


    /*---CREATE SERVER----*/
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


    /*---CREATE SHARED MEMORY, SEMAPHORE AND MESSAGE QUEUES---*/

    //sm für rc
    id = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

    if (id < 0) {
        printf("Fehler bei key %d, mit der size %d\n", IPC_PRIVATE, SEGSIZE);
    }

    rc = shmat(id, NULL, 0);
    *rc = 0;

    //sm für Daten
    id2 = shmget(IPC_PRIVATE, sizeof(data), IPC_CREAT | 0777);

    if (id2 < 0) {
        printf("Fehler bei key %d, mit der size %d\n", IPC_PRIVATE, SEGSIZE);
    }

    sm = (data *) shmat(id2, NULL, 0);

    //sm für subsCount
    id6 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

    if (id6 < 0) {
        printf("Fehler bei key %d, mit der size %d\n", IPC_PRIVATE, SEGSIZE);
    }

    subsCount = (int *) shmat(id6, NULL, 0);

    //message queues
    id3 = msgget(IPC_PRIVATE, IPC_CREAT | 0777);

    //semaphore
    sem_id = semget(IPC_PRIVATE, 2, IPC_CREAT | 0777);

    if (sem_id == -1) {
        perror("Die Gruppe konnte nicht angelegt werden!");
        exit(1);
    }

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


    /*SERVER LOOP*/

    while (1) {
        //--FORK 1--
        fd = accept(sock, (struct sockaddr *) &client, &client_len);
        pid = fork();

        if (pid < 0) {
            perror("ERROR on fork 1");
            exit(1);

        } else if (pid > 0) {
            close(fd);

        } else if (pid == 0) {

            /*SHARED MEMORY FOR PUB/SUB*/
            //sm für subs / to save subbed keys
            id4 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

            if (id4 < 0) {
                printf("Fail key %d with size %d\n", IPC_PRIVATE, SEGSIZE);
            }

            subs = (int *) shmat(id4, NULL, 0);


            //sm für aboCount / to save the number of subs for each key
            id5 = shmget(IPC_PRIVATE, sizeof(int), IPC_CREAT | 0777);

            if (id5 < 0) {
                printf("Fail key %d with size %d\n", IPC_PRIVATE, SEGSIZE);
            }

            aboCount = shmat(id5, NULL, 0);

            //--FORK 2--
            pid2 = fork();

            if (pid2 < 0) {
                perror("ERROR on fork 2");
                exit(1);

            } else if (pid2 > 0) {
                close(sock);

                while ((read_size = recv(fd, in, 2000, 0)) > 0) {
                    strtoken(in, del, tok, 3);

                    printf("\n---------------------------------------\n");

                    if (strcmp(tok[0], "put") == 0) {

                        if (*rc == 0) {
                            semop(sem_id, &down_w, 1);
                            //!-enter critical area-!
                            PUT(tok[1], tok[2], temp, sm, subs, temp2);
                            //!-leave critical area-!
                            semop(sem_id, &up_w, 1);


                            if (sub != 10) {
                                m.mtype = sub + 1;
                                char str[100];
                                snprintf(str, sizeof(str), "\nABO NOTIFICATION: key '%s' with new value '%s' ", sm[sub].key, sm[sub].value);
                                strcpy(m.mtext, str);

                                int v;
                                for (v = 0; v < subsCount[sub]; v++) {
                                    if (msgsnd(id3, &m, 100, 0) < 0) {
                                        perror("msgsnd failed");
                                        exit(1);
                                    }
                                }
                            }
                        } else {
                            printf("%d readers active - access denied !\n", *rc);
                        }
                    } else if (strcmp(tok[0], "get") == 0) {
                        semop(sem_id, &down_r, 1); //erhalte Zugriff auf rc
                        *rc += 1;
                        if (*rc == 1) {                   //erste Leser verweigert Zugriff für Schreibprozesse
                            semop(sem_id, &down_w, 1);
                            printf("first reader - access granted\n");
                        }
                        semop(sem_id, &up_r, 1);  //Zugriff auf rc freigeben
                        printf("add reader - count: %d\n", *rc);
                        GET(tok[1], temp, sm);

                        semop(sem_id, &down_r, 1);  //erhalte Zugriff auf rc
                        *rc -= 1;
                        if (*rc == 0) {                   //letzte Leser erlaubt Zugriff für Schreibprozesse
                            semop(sem_id, &up_w, 1);
                            printf("\nlast reader - free access for writers\n");
                        }
                        semop(sem_id, &up_r, 1);        //Zugriff auf rc freigeben

                    } else if (strcmp(tok[0], "del") == 0) {
                        semop(sem_id, &down_w, 1);
                        DEL(tok[1], sm);
                        semop(sem_id, &up_w, 1);

                    } else if (strcmp(tok[0], "sub") == 0) {
                        int a = 0;
                        int i;

                        for (i = 0; i < STORELENGTH; i++) {
                            if (strcmp(sm[i].key, tok[1]) == 0) {
                                subs[count] = i;
                                char str2[100];
                                snprintf(str2, sizeof(str2), "sub for key %s successful !\n", sm[i].key);
                                subsCount[i]++;
                                strcpy(out, str2); //res in out
                                write(fd, out, strlen(out)); //out ausgeben
                                bzero(out, sizeof(out)); //out auf 0 setzen
                                a = 1;
                                *aboCount = *aboCount + 1;
                                count++;
                            }

                        }

                        if (a == 0) {
                            printf("\nkey not found !\n", tok[1]);
                        }

                    } else if (strcmp(tok[0], "showsubs") == 0) {

                        int y;
                        for (y = 0; y < count; y++) {
                            printf("\n%d sub with key %s\n", y + 1, sm[subs[y]].key);
                        }


                    } else if (strcmp(tok[0], "close") == 0) {
                        semctl(sem_id, 0, IPC_RMID);
                        semctl(sem_id, 1, IPC_RMID);
                        shmctl(id, IPC_RMID, 0);
                        shmctl(id2, IPC_RMID, 0);
                        shmctl(id4, IPC_RMID, 0);
                        shmctl(id6, IPC_RMID, 0);
                        shmctl(id5, IPC_RMID, 0);
                        msgctl(id3, IPC_RMID, NULL);
                        shutdown(fd, 2);
                    } else {
                        printf("\nwrong input !\n");
                    }

                    bzero(in, sizeof(in)); //in auf 0 setzen
                    strcpy(out, res); //res in out
                    bzero(res, sizeof(res)); //res auf 0 setzen
                    write(fd, out, strlen(out)); //out ausgeben
                    bzero(out, sizeof(out)); //out auf 0 setzen

                }

                close(fd);

                exit(0);
            } else if (pid2 == 0) {
                //child-process / asynchronous loop for messages
                while (1) {
                    int i, r = 0;
                    for (i = 0; i < *aboCount; ++i) {
                        if (msgrcv(id3, &m, 100, subs[i] + 1, IPC_NOWAIT) < 0) {
                        } else {
                            strcpy(out, m.mtext); //res in out
                            write(fd, out, strlen(out)); //out ausgeben
                            bzero(out, sizeof(out)); //out auf 0 setzen
                            r = 1;
                        }
                    }
                }
            }
        }
    }
}




