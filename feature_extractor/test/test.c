int data1[500];
int data2[500] = {1,};
int data3[500];

extern void ext(int);

void foo() {
  int i;
  for (i = 0; i < 500; i++) {
    data1[i] = i;
    int *data_ptr = data2 + i;
    data3[i] = *data_ptr;
    ext(data2[i] + 5);
  }
}
