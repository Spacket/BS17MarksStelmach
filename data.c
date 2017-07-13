#include "data.h"
#include <stdio.h>
#include <string.h>

//put a new pair in data or overwrite an existing one
int PUT(char *key, char *value, char *res, data *sm, int a[], int *temp2) {
    int i, j, k;
    for (i = 0; i < STORELENGTH; i++) {
        if (strcmp(sm[i].key, key) == 0) {
            strncpy(res, sm[i].value, STORELENGTH);
            strcpy(sm[i].value, value);
            printf("\nkey already exists --> new value ist: ");

            *temp2 = i;
            return 0;
        }
    }

    for (j = 0; j < STORELENGTH; j++) {
        if (strcmp(sm[j].key, "") == 0) {
            printf("found place on index %i\n", j);
            strcpy(sm[j].value, value);
            strcpy(sm[j].key, key);
            *temp2 = 10;
            return 0;
        }
    }
    printf("\n\nthere is not enough space !\n\n");
    return -1;

}


//get a pair
int GET(char *key, char *res, data *sm) {
    int i;

    for (i = 0; i < STORELENGTH; i++) {
        if (strcmp(sm[i].key, key) == 0) {
            strcpy(res, sm[i].value);
            printf("\nvalue is: ");
            return 0;
        }
    }
    printf("\n\nkey not found !\n\n");
    return -1;
}

//delete a pair
int DEL(char *key, data *sm) {
    int i;
    int j;
    for (i = 0; i < STORELENGTH; i++) {
        if (strcmp(sm[i].key, key) == 0) {
            strcpy(sm[i].value, "");
            strcpy(sm[i].key, "");

            printf("\nvalue deleted, array: \n");

            for (j = 0; j < STORELENGTH; j++) {
                printf("%i KEY is: %s\n", j+1, sm[j].key);
                printf("%i VALUE is: %s\n", j+1, sm[j].value);
            }

            return 0;
        }
    }
    printf("\nkey not found !\n");
    return -1;
}


int strtoken(char *str, char *separator, char **token, int size) {
    int i = 0;
    token[0] = strtok(str, separator);
    while (token[i++] && i < size)
        token[i] = strtok(NULL, separator);
    return (i);
}

