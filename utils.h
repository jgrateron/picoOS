#include <string.h>
#include <ctype.h>
#include <strings.h>

#define MAX_LEN_COMMAND 40

bool strequal(const char * arg1, const char * arg2){
    if (strcmp(arg1,arg2) == 0){
        return true;
    }
    else {
        return false;
    }
}

bool isnumber(const char * arg){
    for (int i = 0; i < strlen(arg); i++){
        if (!isdigit(arg[i])){
            return false;
        }
    }
    return true;
}

void readcommand(char command[]){
    int pos = 0;
    while(true){
        char c = getchar();
        if (c == 13){
            printf("\n");
            break;
        }
        if (c == 127){
            if(strlen(command) > 0){
                printf("\033[1D");
                printf(" ");
                printf("\033[1D");
                command[strlen(command) -1] = '\0';
                pos--;
            }
        }
        if (c >= 32 && c <= 125){
            printf("%c", c);
            command [pos] = c;
            pos++;
            if (pos == MAX_LEN_COMMAND){
                printf("\n");
                break;
            }
        }
    }
}

bool not(bool condition){
    return !condition;
}