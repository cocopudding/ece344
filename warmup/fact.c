#include "common.h"
#include <stdlib.h> 


int main(int argc, char *argv[])
{
    double f;
    int i,factorial,a;
    factorial=1;
    
    if((argc!=2)){
        printf("Huh?\n");
        return 0;
    }
    
    f= atof (argv[1]);
    i= atoi (argv[1]);
    
    if((f==0)||(f!=i)){
        printf("Huh?\n");
    }
    else if(f>12||i>12){
        printf("Overflow\n");
    }
    else{
        for(a=1;a<=i;a++){
            factorial=factorial*a;
        }
        printf("%d\n",factorial);
    }
	return 0;
}
