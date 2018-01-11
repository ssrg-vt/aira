/* Super simple test
 * 5/21/2014
 * Author: bielsk1
 */
#define NONZER 23
#define NA 7
static int acol[24] = {0,};
double cuf[NA+1];
int qww  [NONZER+1] = {2,};
#pragma omp threadprivate(cuf,qww)

void foo(int array[], int s);

/*MAINNNNN*/
int main(int argc, char* argv){
	int s;

	for(s=0;s <8 ;s++){
		cuf[s] = 2;
	}
	printf("num threads: %d\n",omp_get_num_threads());
	
	foo( acol, 3 );
	printf("num threads: %d\n",omp_get_num_threads());

        for(s=0; s<24; s++){
                printf("number in x[%d]: %d\n",s,acol[s]);
        }
        for(s=0; s<24; s++){
                printf("number in x[%d]: %d\n",s,acol[s]);
        }
        for(s=0; s<24; s++){
                printf("number in x[%d]: %d\n",s,acol[s]);
        }
        for(s=0; s<24; s++){
                printf("number in x[%d]: %d\n",s,acol[s]);
        }
        for(s=0; s<24; s++){
                printf("number in x[%d]: %d\n",s,acol[s]);
        }
        return 0;

}/*END MAINNNN*/

void foo(int EEE[], int s){
	int i,j;
	printf("num threads: %d\n",omp_get_num_threads());
	#pragma omp for 
	for(j =0; j < 3 ; j++){
		EEE[j]= 33 - s +qww[j];
	}
}
