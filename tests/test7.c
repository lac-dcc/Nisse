char* last(char* s) {
    char* p = s;
    while (*p != '\0') {
        p++;
    }
    return p;
}

int* lastint(int* s) {
    int* p = s;
    while (*p != 0) {
        p++;
    }
    return p;
}

int main(int argc, char** argv) {
    int test[] = {1,2,3,0};
    last("Hello");
    lastint(test);
    return 0;
}