#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "rand_dist.h"

int main (void){
    double* poisson_arrival = (double *)malloc( 1000* sizeof(double) );
    double rate = 2000.0;
    GenPoissonArrival(rate, 1000, poisson_arrival);
    for(int n = 0; n < 1000; n++){  
        printf("%" PRIu64 ", %.5lf\n", (uint64_t) round(poisson_arrival[n]), poisson_arrival[n]);
        //printf ("%.5f\n", poisson_array[i]*1000000.0);
    }

    return 0;
}
