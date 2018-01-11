/* Super simple test
 * 5/21/2014
 * Author: bielsk1
 */
#define NONZER 23
#define NA 7
static int acol[24UL] = {(0)};
double cuf[7 + 1];
int qww[24UL] = {(2)};
#pragma omp threadprivate ( cuf, qww )
void foo(int array[],int s);
/*MAINNNNN*/
void OUT__3__7847__(int *sp__);

int main(int argc,char *argv)
{
  int s;
  OUT__3__7847__(&s);
  foo(acol,3);
  return 0;
/*END MAINNNN*/
}
void OUT__1__7847__(int *jp__,int *rp__);
void OUT__2__7847__(int *ip__);

void foo(int EEE[],int s)
{
  int i;
  int j;
  OUT__2__7847__(&i);
  j = 50;
  i = (s + j);
//printf("num: %d",i);
  int r;
  OUT__1__7847__(&j,&r);
//printf("num threads: %d\n",omp_get_num_threads());
}

void OUT__1__7847__(int *jp__,int *rp__)
{
  int *j = (int *)jp__;
  int *r = (int *)rp__;
  for ( *r = 0;  *r < 8; ( *r)++) {
     *j += 23;
  }
}

void OUT__2__7847__(int *ip__)
{
  int *i = (int *)ip__;
  for ( *i = 0;  *i < 2 * 7; ( *i)++) {
    acol[ *i] = (-1.0e99);
  }
}

void OUT__3__7847__(int *sp__)
{
  int *s = (int *)sp__;
  for ( *s = 0;  *s < 8; ( *s)++) {
    cuf[ *s] = 2;
    int ss;
    int ee;
    int yy;
    int ttsf;
    int ssfsdf;
    int sddfsdf;
//printf("Im in the loopoooooppopoopo\n");
    ss = (cuf[ *s] + 200);
//printf("totlala: %d  llalalala\n",ss);
    yy = (400 - ss);
//printf("more stugg in for lopsopsosp\n");
  }
}
