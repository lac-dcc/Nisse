#include <stdio.h>

int a[] = {0,0,1};

int aux() {
    for (int i = 0; i<10; i++) {
        a[0]++;
        a[1]--;
        a[2]<<=1;
    }
    return 0;
}

int main() {
    printf("%d %d %d\n", a[0], a[1], a[2]);
    aux();
    printf("%d %d %d\n", a[0], a[1], a[2]);
    aux();
    printf("%d %d %d\n", a[0], a[1], a[2]);
    return 0;
}