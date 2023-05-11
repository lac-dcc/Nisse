int main() {
    int n = 100;
    int x = 0;
    for(int i = 0; i<n; i++) {
        for (int j = 0; j<12; j++) {
            x--;
        }
        for (int k = 12; k<i; k++) {
            ;
        }
    }
    return 0;
}