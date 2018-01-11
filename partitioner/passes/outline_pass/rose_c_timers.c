#include "wtime.h"
#include <stdlib.h>
#ifdef _OPENMP
#include <omp.h>
#endif
/*  Prototype  */
void wtime_(double *);
/*****************************************************************/
/******         E  L  A  P  S  E  D  _  T  I  M  E          ******/
/*****************************************************************/

static double elapsed_time()
{
  double t;
#if defined(_OPENMP) && (_OPENMP > 200010)
/*  Use the OpenMP timer if we can */
#else
  wtime_(&t);
#endif
  return t;
}
static double start[64UL];
static double elapsed[64UL];
static unsigned int count[64UL];
#ifdef _OPENMP
#endif
/*****************************************************************/
/******            T  I  M  E  R  _  C  L  E  A  R          ******/
/*****************************************************************/

void timer_clear(int n)
{
  elapsed[n] = 0.0;
  count[n] = 0;
}
/*****************************************************************/
/******            T  I  M  E  R  _  S  T  A  R  T          ******/
/*****************************************************************/

void timer_start(int n)
{
  start[n] = elapsed_time();
}
/*****************************************************************/
/******            T  I  M  E  R  _  S  T  O  P             ******/
/*****************************************************************/

void timer_stop(int n)
{
  double t;
  double now;
  now = elapsed_time();
  t = (now - start[n]);
  elapsed[n] += t;
  count[n]++;
}
/*****************************************************************/
/******            T  I  M  E  R  _  R  E  A  D             ******/
/*****************************************************************/

double timer_read(int n)
{
  return elapsed[n];
}

unsigned int timer_count(int n)
{
  return count[n];
}
