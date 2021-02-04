#include <stdio.h>
#include <unistd.h>
#include <inttypes.h>
#include "rand_dist.h"

// compile this file:
// gcc test_rand_dist.c -lgsl -lgslcblas -lm -o output
int main (void){
    uint64_t* poisson_arrival = (uint64_t *)malloc( 60*30000* sizeof(uint64_t) );
    double rate = 30000.0;
    GenPoissonArrival(rate, 60*30000, poisson_arrival);
    for(int n = 0; n < 60*30000; n++){  
        //printf("%" PRIu64 ", %.5lf\n", (uint64_t) round(poisson_arrival[n]), poisson_arrival[n]);
        printf("%" PRIu64 "\n",  poisson_arrival[n]);
    }

    return 0;
}
