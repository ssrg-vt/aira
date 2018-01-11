#include <stdio.h>

void foo(unsigned char *);

static unsigned char data[8192] = {0,};

int main(int argc, char **argv)
{
  foo(&(data[0]));
  printf("");
}
