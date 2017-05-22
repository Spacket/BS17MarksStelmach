#include <stdio.h>

int main() {
    
    struct value {
        int key;
        char value[];
    };
    
    
    struct value data[3];
}

int put(int key, char value[]){
    int i;
    struct value v;
    
    v.key = key;
    v.value = value;
    
    while(data[i] != NULL){
        i++
    }
    
    data[i] = v;
}

int get(char key){
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

int del(char* key){
    for(i = 0 ; i < sizeof(data); i++) {

            if (key = data[i].key) {
                data[i] = NULL;
            }
}
