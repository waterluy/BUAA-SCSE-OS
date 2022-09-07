#include <stdio.h>

int main() {
    printf("%4s\n","abcdef");
    printf("%0c\n",'d');
    //printf("\\%%\n");
    //printf("\\%-=.6ll\n");
    //printf("\\%.ld\n",9);
    long int a = 4;
    printf("%-0-0-0-0dddd\n",9,12);
    printf("%d\n",(a<0));
    printf("%ld\n",a);
    
    printf("\\%l0-8d\n",9);
    printf("%3c",'d');
    return 0;
}