#include <stdio.h>

int main() {

    int data[3];

    struct data {
        int key;
        char value[];
    };
}

int put(int key, char value[]){
    struct data d;
    d.key = key;
    d.value = value;
}

int get(char key, char res){
    char searched[];
    int i;

    for(i = 0 ; i < sizeof(data); i++) {

            if (key = data[i].key) {
                searched = data[i].value;
            }

    }

    if(searched != NULL){
        return searched;
    }

    else{
        return 0;
    }
}

int del(char* key, char* res){

}