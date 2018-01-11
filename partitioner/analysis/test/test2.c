void foo(unsigned char *data)
{
  data[0] = 5;
  data[4096] = data[1];
}
