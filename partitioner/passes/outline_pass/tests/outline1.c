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

	
	#pragma rose_outline
	for(s=0;s <8 ;s++){
		cuf[s] = 2;
		int ss, ee,yy, ttsf,ssfsdf,sddfsdf;
		//printf("Im in the loopoooooppopoopo\n");
		ss = cuf[s] +200;
		//printf("totlala: %d  llalalala\n",ss);
		yy = 400 - ss;
		//printf("more stugg in for lopsopsosp\n");
	}
	
	foo( acol, 3 );

        return 0;
}/*END MAINNNN*/

void foo(int EEE[], int s){
	int i,j;

	
	//used to be ...
	//#pragma omp parallel default(shared) private(i)
	//{ etc....
	// ...
	//....
	//}

	//now \/	
	#pragma rose_outline
	{
		for (i = 0; i < 2 * NA; i++) {
			acol[i] = -1.0e99;
		}
	}

	j = 50;
	i = s + j;
	//printf("num: %d",i);


	int r;
	#pragma rose_outline
	for(r=0;r <8 ;r++){
		j += NONZER;
	}
	//printf("num threads: %d\n",omp_get_num_threads());
}
