#include "common.h"

void trim(char *s){
    int right = strlen(s) - 1, left = 0, idx = 0;
    while (right >=0 && s[right] == ' ') right--;
    while (left < strlen(s) && s[left] == ' ') left++;
    s[right+1] = '\0';
    while (left <= right + 1){
        s[idx++] = s[left++];
    }
}

void strupr(char *str){
    for (; *str!='\0'; str++)
        *str = toupper(*str);
}

