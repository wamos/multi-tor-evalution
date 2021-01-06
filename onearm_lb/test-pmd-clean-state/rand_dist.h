#ifndef RAND_DIST_HEADER_H
#define RAND_DIST_HEADER_H
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

enum {
	GEN_UNIFORM = 0,
	GEN_EXP,
	GEN_BIMODAL,
};

gsl_rng* setup_gsl_rng(){
  const gsl_rng_type * T;
  gsl_rng * r;

  // If you don’t specify a generator for GSL_RNG_TYPE then gsl_rng_mt19937 is used as the default. 
  // The initial value of gsl_rng_default_seed is zero.
  gsl_rng_env_setup();
  T = gsl_rng_default;
  r = gsl_rng_alloc (T);

  return r;
}

void GenPoissonArrival(double lambda, uint32_t size, double* poisson_array) {
  // Generate and output exponential random variables
  gsl_rng* rng = setup_gsl_rng();
  double mu = 1.0/lambda;

  for (uint32_t i = 0; i < size; i++){
    poisson_array[i] = gsl_ran_exponential(rng , mu);
    poisson_array[i] = poisson_array[i]*1000000.0;
    //printf ("%.5f\n", poisson_array[i]*1000000.0);
  }

  gsl_rng_free (rng);
}

#endif
