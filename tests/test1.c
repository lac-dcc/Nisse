int test1() {
  int x = 0;
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 20; j++) {
      x++;
      if (x > 10)
        return x;
    }
  }
  return x;
}

int test2(int x) {
  while (x > 0) {
    x--;
  }
  return 0;
}

int main() {
  for (int i = 0; i<5; i++) {
    test2(i);
  }
  test1();
  return 0;
}