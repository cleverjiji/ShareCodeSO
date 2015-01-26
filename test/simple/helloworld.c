#include <stdio.h>
int main(int argc, const char *argv[])
{
    switch(argc){
        case 1: printf("argc=1\n");break;
        case 2: printf("argc=2\n");break;
        case 3: printf("argc=3\n");break;
        case 4: printf("argc=4\n");break;
    }
    printf("End Switch\n");
    return 0;
}
