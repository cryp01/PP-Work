#include <omp.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


float stencil ( float v1, float v2, float v3, float v4)
{
  return (v1 + v2 + v3 + v4) * 0.25f;
}

float max_error ( float prev_error, float old, float new )
{
  float t= fabsf( new - old );
  return t>prev_error? t: prev_error;
}

float laplace_step(float *in, float *out, int n)
{
  int i, j;
  float error=0.0f;
  #pragma omp for
  for ( j=1; j < n-1; j++ )
  #pragma omp simd reduction(max:error)
    for ( i=1; i < n-1; i++ )
    {
      out[j*n+i]= stencil(in[j*n+i+1], in[j*n+i-1], in[(j-1)*n+i], in[(j+1)*n+i]);
      error = max_error( error, out[j*n+i], in[j*n+i] );
    }
  return error;
}


void laplace_init(float *in, int n)
{
  int i;
  float V;
  const float pi  = 2.0f * asinf(1.0f);
  memset(in, 0, n*n*sizeof(float));
    for (i=0; i<n; i++) {
    V = in[i*n] = sinf(pi*i / (n-1));
    in[ i*n+n-1 ] = V*expf(-pi);
  }
}

int main(int argc, char** argv)
{
  int n = 4096;
  int iter_max = 1000;
  float *A, *temp;
    
  const float tol = 1.0e-5f;
  float error= 1.0f;    
  // get runtime arguments 
  if (argc>1) {  n        = atoi(argv[1]); }
  if (argc>2) {  iter_max = atoi(argv[2]); }

  A    = (float*) malloc( n*n*sizeof(float) );
  temp = (float*) malloc( n*n*sizeof(float) );

  //  set boundary conditions
  laplace_init (A, n);
  laplace_init (temp, n);
  A[(n/128)*n+n/128] = 1.0f; // set singular point

  printf("Jacobi relaxation Calculation: %d x %d mesh,"
         " maximum of %d iterations\n", 
         n, n, iter_max );
 
  int iter = 0;  
  #pragma omp parallel firstprivate(A,temp,iter) 
  while ( error > tol*tol && iter < iter_max )
  {
    iter++;
    error= laplace_step (A, temp, n);
    float *swap= A; A=temp; temp= swap; // swap pointers A & temp
  }
  error = sqrtf( error );
  printf("Total Iterations: %5d, ERROR: %0.6f, ", iter, error);
  printf("A[%d][%d]= %0.6f\n", n/128, n/128, A[(n/128)*n+n/128]);

  free(A); free(temp);
}
