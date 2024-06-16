#include <stdio.h>
#include <string.h>

int main(){

    char string[500];
    do{
        printf("Digite: ");
        scanf("%s",string);
    }while(strcmp("/q",string));

    return 0;
}