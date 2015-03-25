#include <stdio.h>
void foo(){
    __asm__ __volatile__(
            "call   *(%%rax) \n\t"
            :::
            );
}
int main(int argc, const char *argv[])
{
    switch(argc){
        case 1: printf("argc=1\n");break;
        case 2: printf("argc=2\n");break;
        case 3: printf("argc=3\n");break;
        case 4: printf("argc=4\n");break;
    }
    printf("End Switch\n");
    while(1);
    return 0;
}
