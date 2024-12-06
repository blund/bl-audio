/*	
	A set of utility programs to compute the Fast Fourier Transform (FFT):
	
					       N-1
					X[k] = SUM { x[n]exp(-j2(pi)nk/N) }
					       n=0
	
	and inverse Fast Fourier Transform (iFFT):
	
					           N-1
					x[n] = 1/N SUM { X[k]exp(+j2(pi)nk/N) }
					           k=0
	
	To speed things up, multiplication by 1 and j are avoided.  The input, x[],
	is an array of complex numbers (pairs of doubles) of length N = 2^p.  The
	calling program supplies p = log2(N) not the array length, N.  The output
	is placed in BIT REVERSED order in x[].  Call bitreverse(x, p) to swap back
	to normal order, if needed.  However, iFFT(X, p, trigtbl) requires its input,
	X[], to be in bit reversed order making bit reversing unnecessary in some
	cases, such as fast convolution.  trigtbl is an array of doubles of length
	N/4 containing the sin function from 0 to pi/2 used to compute the FFT.
	Call sintable(trigtbl, p) ONLY ONCE before either FFT(x, p, trigtbl) or
	iFFT(X, p, trigtbl) to set up a sin table for FFT computation.
	
	Written in Megamax C (for the Mac) by Robert Bristow-Johnson (1985).
*/

/*
#include <complex.h>
*/

typedef struct {
	double real;
	double imag;
} complex;

#define Re(z) (z).real
#define Im(z) (z).imag

#define PI (3.1415926535897932587)

double sin();

FFT (x, p, trigtbl)
complex x[];
int p;
register double *trigtbl;
{
	register long length, step, stepsize, size;
	register complex *top, *bottom, *end;				/* top & bottom of FFT butterfly */
	complex temp;
	
	size = 1L<<p;
	end = x + size;
	
	length = size>>1;	
	size >>= 2;
	stepsize = 1L;
	while ( length > 1L) {
		top = x;
		while (top < end) {
			bottom = top + length;
			
			Re(temp) = Re(*top) - Re(*bottom);			/* butterfly: twiddle = 1 */
			Im(temp) = Im(*top) - Im(*bottom);
			Re(*top) += Re(*bottom);
			Im(*top) += Im(*bottom);
			*bottom = temp;
			top++;
			bottom++;
			
			for (step = stepsize; step < size; step += stepsize) {
				Re(temp) = Re(*top) - Re(*bottom);		/* butterfly: twiddle = exp(-jí) */
				Im(temp) = Im(*top) - Im(*bottom);
				Re(*top) += Re(*bottom);
				Im(*top) += Im(*bottom);
				Re(*bottom) = Re(temp)*trigtbl[size - step] + Im(temp)*trigtbl[step];
				Im(*bottom) = Im(temp)*trigtbl[size - step] - Re(temp)*trigtbl[step];
				top++;
				bottom++;
			}
			
			Re(temp) = Im(*top) - Im(*bottom);			/* butterfly: twiddle = -j */
			Im(temp) = Re(*bottom) - Re(*top);
			Re(*top) += Re(*bottom);
			Im(*top) += Im(*bottom);
			*bottom = temp;
			top++;
			bottom++;
			
			for (step = stepsize; step < size; step += stepsize) {
				Re(temp) = Im(*top) - Im(*bottom);		/* butterfly: twiddle = -j*exp(-jí) */
				Im(temp) = Re(*bottom) - Re(*top);
				Re(*top) += Re(*bottom);
				Im(*top) += Im(*bottom);
				Re(*bottom) = Re(temp)*trigtbl[size - step] + Im(temp)*trigtbl[step];
				Im(*bottom) = Im(temp)*trigtbl[size - step] - Re(temp)*trigtbl[step];
				top++;
				bottom++;
			}
			top = bottom;
		}
		stepsize <<= 1;
		length >>= 1;
	}
	
	top = x;
	bottom = x + 1;
	while (top <  end) {
		Re(temp) = Re(*top) - Re(*bottom);				/* butterfly: twiddle = 1 */
		Im(temp) = Im(*top) - Im(*bottom);
		Re(*top) += Re(*bottom);
		Im(*top) += Im(*bottom);
		*bottom = temp;
		top += 2;
		bottom += 2;
	}
}


iFFT (X, p, trigtbl)
complex X[];
int p;
register double *trigtbl;
{
	register long length, step, stepsize, size;
	double scale;
	register complex *top, *bottom, *end;				/* top & bottom of FFT butterfly */
	complex temp;
	
	size = 1L<<p;
	end = X + size;
	
	scale = 1.0/(double)size;
	top = X;
	bottom = X + 1;
	while (top <  end) {
		Re(temp) = (Re(*top) - Re(*bottom))*scale;		/* butterfly: twiddle = 1/N */
		Im(temp) = (Im(*top) - Im(*bottom))*scale;
		Re(*top) = (Re(*top) + Re(*bottom))*scale;
		Im(*top) = (Im(*top) + Im(*bottom))*scale;
		*bottom = temp;
		top += 2;
		bottom += 2;
	}
	
	length = 1L;
	size >>= 2;
	stepsize = size;
	while ( stepsize >= 1L) {
		length <<= 1;
		top = X;
		while (top < end) {
			bottom = top + length;
			
			temp = *bottom;								/* butterfly: twiddle = 1 */
			Re(*bottom) = Re(*top) - Re(temp);
			Im(*bottom) = Im(*top) - Im(temp);
			Re(*top) += Re(temp);
			Im(*top) += Im(temp);
			top++;
			bottom++;
			
			for (step = stepsize; step < size; step += stepsize) {
														/* butterfly: twiddle = exp(+jí) */
				Re(temp) = Re(*bottom)*trigtbl[size - step] - Im(*bottom)*trigtbl[step];
				Im(temp) = Im(*bottom)*trigtbl[size - step] + Re(*bottom)*trigtbl[step];
				Re(*bottom) = Re(*top) - Re(temp);
				Im(*bottom) = Im(*top) - Im(temp);
				Re(*top) += Re(temp);
				Im(*top) += Im(temp);
				top++;
				bottom++;
			}
			
			Re(temp) = -Im(*bottom);					/* butterfly: twiddle = +j */
			Im(temp) = Re(*bottom);
			Re(*bottom) = Re(*top) - Re(temp);
			Im(*bottom) = Im(*top) - Im(temp);
			Re(*top) += Re(temp);
			Im(*top) += Im(temp);
			top++;
			bottom++;
			
			for (step = stepsize; step < size; step += stepsize) {
														/* butterfly: twiddle = +j*exp(+jí) */
				Re(temp) = -Im(*bottom)*trigtbl[size - step] - Re(*bottom)*trigtbl[step];
				Im(temp) = Re(*bottom)*trigtbl[size - step] - Im(*bottom)*trigtbl[step];
				Re(*bottom) = Re(*top) - Re(temp);
				Im(*bottom) = Im(*top) - Im(temp);
				Re(*top) += Re(temp);
				Im(*top) += Im(temp);
				top++;
				bottom++;
			}
			top = bottom;
		}
		stepsize >>= 1;
	}
}


sintable(trigtbl, p)
register double *trigtbl;
int p;
{
	register long size, i;
	double theta;
	
	size = 1L<<(p-2);
	theta = (PI/2.0)/(double)size;
	
	for (i = 0; i < size; i++) 
		trigtbl[i] = sin(theta*(double)i);
}


bitreverse (x, p)
register complex *x;
int p;
{
	complex temp;
	register long k, i, r, size, count;
	
	size = (1L<<p) - 1L;
	for (k = 1L; k < size; k++) {
		i = k;
		r = 0;
		for (count = size; count > 0; count >>= 1) {
			r <<= 1;
			r += (i & 0x00000001L);
			i >>= 1;
		}
		if (r > k) {
			temp = x[r];
			x[r] = x[k];
			x[k] = temp;
		}
	}
}
