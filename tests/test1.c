int test1() {
  int x = 0;
  for (int i = 0; i < 10; i++) {
    for (int j = 0; j < 20; j++) {
      x++;
      if (x > 10)
        return x;
    }
  }
  return 3;
}

int test3(int x) {
  int i = 100;
  while (x<10) {
    if (i%2==0) {
      x++;
      i /= 2;
    } else {
      i = 3*i+1;
    }
  }
  return 0;
}

int test2(int x) {
  while (x > 0) {
    x--;
  }
  return 0;
}

int main() {
  for (int i = 0; i<10; i++) {
    test2(i);
  }
  test3(0);
  test1();
  return 0;
}