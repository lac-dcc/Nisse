int main() {
    int n = 100;
    l:
    n--;
    for (int x = 0; x<n; x++) {
        if (n>50)
            goto l;
        ;
    }
    return 0;
}