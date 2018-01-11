// ! o u t l i n e . cc : Demonstrates t h e pragma−i n t e r f a c e o f t h e O u t l i n e r .
#include <rose.h>
#include <iostream>

#include <Outliner.hh>
#include <vector>
#include <string>
using namespace std;


int main ( int argc , char* argv[] )
{
// ! A c c e p t i n g command l i n e o p t i o n s t o t h e o u t l i n e r
vector <string> argvList ( argv , argv+argc ) ;
Outliner::commandLineProcessing ( argvList ) ;
SgProject *proj = frontend ( argvList ) ;
ROSE_ASSERT( proj != NULL ) ;

printf("[ O u t l i n i n g . . . ] \n");
size_t count = Outliner::outlineAll(proj);
printf("[ P r o c e s s e d ” <<%d << ” o u t l i n e",count);
return backend(proj);
}

