//
//  filter.c
//
#include <math.h>
#include "filter.h"

void computeSectionCofficients(float Fs, enum EQ_type type, float f0, float dBgain, float Q, sectionData* data)
{
	float temp, *coef_ptr;
	float A = expf(ONE_OVER_DB_LOG_AMPL*dBgain);
	float A2 = A*A;
	float Ap1, Am1;
	float cos_w0 = cosf(TWOPI*f0/Fs);
	float sin_w0 = sinf(TWOPI*f0/Fs);
	float alpha = 0.5f*sin_w0/Q;
	float a0, a1, a2, b0, b1, b2;
	
	switch (type)
    {
		case LPF:
			b0 =  0.5f*(1.0f - cos_w0)*A2;
			b1 =  2.0f*b0;
			b2 =      b0;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case HPF:
			b0 =  0.5f*(1.0f + cos_w0)*A2;
			b1 = -2.0f*b0;
			b2 =      b0;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case BPF:
			b0 =   0.5f*sin_w0*A2;
			b1 =   0.0f;
			b2 =  -b0;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case BPF0:
			b0 =   alpha*A2;
			b1 =   0.0f;
			b2 =  -b0;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case notch:
			b0 =   A2;
			b1 =  -2.0f*cos_w0*A2;
			b2 =   A2;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case APF:
			b0 =  (1.0f - alpha)*A2;
			b1 =  -2.0f*cos_w0*A2;
			b2 =  (1.0f + alpha)*A2;
			a0 =   1.0f + alpha;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha;
			break;
            
		case peakingEQ:
			if (A > 1.0)
            {
				alpha *= A;
            }
            else
            {
				alpha /= A;
            }
			b0 =   1.0f + alpha*A;
			b1 =  -2.0f*cos_w0;
			b2 =   1.0f - alpha*A;
			a0 =   1.0f + alpha/A;
			a1 =  -2.0f*cos_w0;
			a2 =   1.0f - alpha/A;
			break;
            
		case lowShelf:
			temp = 2.0f*sqrtf(A)*alpha;
			Ap1  = A + 1.0f;
			Am1  = A - 1.0f;
			b0 =  A*(Ap1 - Am1*cos_w0 + temp);
			b1 =  2.0f*A*(Am1 - Ap1*cos_w0);
			b2 =  A*(Ap1 - Am1*cos_w0 - temp);
			a0 =  Ap1 + Am1*cos_w0 + temp;
			a1 =  -2.0f*( Am1 + Ap1*cos_w0);
			a2 =  Ap1 + Am1*cos_w0 - temp;
			break;
            
		case highShelf:
			temp = 2.0f*sqrtf(A)*alpha;
			Ap1  = A + 1.0f;
			Am1  = A - 1.0f;
			b0 =  A*(Ap1 + Am1*cos_w0 + temp);
			b1 = -2.0f*A*(Am1 + Ap1*cos_w0);
			b2 =  A*(Ap1 + Am1*cos_w0 - temp);
			a0 =  Ap1 - Am1*cos_w0 + temp;
			a1 =  2.0f*( Am1 - Ap1*cos_w0);
			a2 =  Ap1 - Am1*cos_w0 - temp;
			break;
            
		case lowShelf1:
			temp = cos_w0 + 1.0f;
			b0 =  A2*sin_w0 +    temp;
			b1 =  A2*sin_w0 -    temp;
			b2 =  0.0f;
			a0 =     sin_w0 +    temp;
			a1 =     sin_w0 -    temp;
			a2 =  0.0f;
			break;
            
		case highShelf1:
			temp = cos_w0 + 1.0f;
			b0 =     sin_w0 + A2*temp;
			b1 =     sin_w0 - A2*temp;
			b2 =  0.0f;
			a0 =     sin_w0 +    temp;
			a1 =     sin_w0 -    temp;
			a2 =  0.0f;
			break;
		
		default:
			b0 = 0.0;
			b1 = 0.0;
			b2 = 0.0;
			a0 = 1.0;
			a1 = 0.0;
			a2 = 0.0;
			break;
    }
	
	coef_ptr = &(data->filterCoefficients[0]);
	temp = 1.0f/a0;
	*coef_ptr++ = temp*b0;						// b0/a0
	*coef_ptr++ = temp*b1;						// b1/a0
	*coef_ptr++ = temp*b2;						// b2/a0
	*coef_ptr++ = -temp*a1;						// -a1/a0
	*coef_ptr   = -temp*a2;						// -a2/a0
	
	coef_ptr = &(data->graphCoefficients[0]);
	temp = b0*b2;
	*coef_ptr++ = temp;							// B2
	*coef_ptr++ = -0.25f*b1*(b0 + b2) - temp;	// B1
	temp = b0 + b1 + b2;
	temp = 0.0625f*temp*temp;
	*coef_ptr++ = temp;							// B0
	temp = a0*a2;
	*coef_ptr++ = temp;							// A2
	*coef_ptr++ = -0.25f*a1*(a0 + a2) - temp;	// A1
	temp = a0 + a1 + a2;
	temp = 0.0625f*temp*temp;
	*coef_ptr   = temp;							// A0
	
	data->Fs = Fs;
	data->type = type;
	data->f0 = f0;
	data->dBgain = dBgain;
	data->Q = Q;
}



void plotSectionFrequencyResponse(int32 Npoints, float Fmin, float Fmax, float* frequencyValues, float* dBvalues, sectionData* data)
{
	int32 n;
	float logFreqStep = logf(Fmax/Fmin)/(float)(Npoints-1);
	float phi, tau = 1.0f/(data->Fs);
	float* coef_ptr = &(data->graphCoefficients[0]);
	float B2, B1, B0, A2, A1, A0;
	
	B2 = *coef_ptr++;
	B1 = *coef_ptr++;
	B0 = *coef_ptr++;
	A2 = *coef_ptr++;
	A1 = *coef_ptr++;
	A0 = *coef_ptr;
	
	for (n=0; n<Npoints; n++)
    {
		phi = Fmin*expf((float)n*logFreqStep);
		frequencyValues[n] = phi;
		phi = sinf(PI*tau*phi);
		phi = phi*phi;								// sin(w/2)^2
		dBvalues[n] += DB_LOG_ENERGY*( logf((B2*phi + B1)*phi + B0) - logf((A2*phi + A1)*phi + A0) );
    }
}


void filterBuffer(int32 Nsamples, int32 Nsections, float* y, float* x, float* filterStates, sectionData** sections)
{
	register int32 n;
	register int32 i;
	register sectionData** section_ptr;
	register float sampleValue, state1, state2, *state_ptr, *coef_ptr;
	
	for (n=0; n<Nsamples; n++)
    {
		section_ptr = sections;						// reset section pointer
		state_ptr = filterStates;					// reset state pointer
        
		sampleValue = x[n];							// get input sample
		
        state2 = *state_ptr++;						//   x[n-2]           
        state1 = *state_ptr--;						//   x[n-1]           
        *state_ptr++ = state1;						//   x[n-1] -> x[n-2] 
        *state_ptr++ = sampleValue;					//   x[n]   -> x[n-1] 
        
		i = Nsections;
		while (--i >= 0)
        {
			coef_ptr = &((*section_ptr++)->filterCoefficients[0]);	// point to section filter coefficients
			
			sampleValue =  *coef_ptr++ * sampleValue;	//   b0*x[n]          
			sampleValue += *coef_ptr++ * state1;		//   b1*x[n-1]        
			sampleValue += *coef_ptr++ * state2;		//   b2*x[n-2]        
			
			state2 = *state_ptr++;						//   y[n-2]           
			state1 = *state_ptr--;						//   y[n-1]           
			*state_ptr++ = state1;						//   y[n-1] -> y[n-2] 
			
			sampleValue += *coef_ptr++ * state1;		//  -a1*y[n-1]        
			sampleValue += *coef_ptr++ * state2;		//  -a2*y[n-2]        
            
			*state_ptr++ = sampleValue;					//   y[n]   -> y[n-1] 

			//     at this point sampleValue is the output of the section just completed
			//     and will be the input to the section about to be computed unless this
			//     is the last section, in which case sampleValue is the output.
			//     state1 and state2 are the output states of the section just completed
			//     and will be the input states of of the section about to be computed.
        }
		
		y[n] = sampleValue;								// store output sample
    }
}



#if 0
                                                        // Orfanidis EQ, simplistic shelf
float a0, a1, a2, b0, b1, b2;
float a[3], b[3];

float cos_w0 = cosf(TWOPI*f0/Fs);
float sin_w0 = sinf(TWOPI*f0/Fs);

float alpha = 0.5*sin_w0/Q;
float A = expf(ONE_OVER_DB_LOG_AMPL*dBgain);


switch (type)
{
    case peakingEQ:
    {
        if (A < 1.000001 && A >= 1.0)
        {
            A = 1.000001;
        }
        else if (A > 0.999999 && A < 1.0)
        {
            A = 0.999999;
        }
        
        float A_2  = A*A;
        
        float A1_2 = Q*(2.0*f0 - 0.5/f0);
		      A1_2 = A1_2*A1_2;
		      A1_2 = (A1_2 + A_2)/(A1_2 + 1.0);
        float A1   = sqrtf(A1_2);
        
        float A_2m1           = A_2 - 1.0;
        float A_2mA1          = A_2 - A1;
        float A1_2m1          = A1_2 - 1.0;
        float A_2mA1_2        = A_2 - A1_2;
        float A_2mA1_2mA1_2m1 = A_2mA1_2 - A1_2m1;
        
        float GG = 1.0/A_2m1;
        float FF = sqrtf(A_2mA1_2*GG);
        float EE = sqrtf(A_2m1/A_2mA1_2mA1_2m1);
        
        float w2 = FF*(1.0-cos_w0)/(1.0+cos_w0);
        
        float CC = (1.0 + EE*w2)*alpha;
		      CC = A_2mA1_2mA1_2m1*CC*CC - w2*(A_2mA1 - A1 + 1.0 - A_2mA1_2mA1_2m1*EE);
        float DD = 2.0*w2*(A_2mA1 - A_2m1*FF);
        
        float AA = sqrtf((    CC +     DD + DD)*GG);
        float BB = sqrtf((A_2*CC + A_2*DD + DD)*GG);
        
        b0 = A1 + w2 + BB;
        b1 = 2.0*(w2 - A1);
        b2 = A1 + w2 - BB;
        a0 = 1.0 + w2 + AA;
        a1 = 2.0*(w2 - 1.0);
        a2 = 1.0 + w2 - AA;
    }
        break;
        
    case lowShelf:
    {
        float beta = 0.5*(1.0 - cosval)*(A - 1.0);
        
        b0 = 1.0 + alpha + beta;
        b1 =  -2.0*(cosval - beta);
        b2 = 1.0 - alpha + beta;
        a0 = 1.0 + alpha;
        a1 = -2.0*cosval;
        a2 = 1.0 - alpha;
    }
        break;
        
    case highShelf:
    {
        float beta = 0.5*(1.0 + cosval)*(A - 1.0);
        
        b0 = 1.0 + alpha + beta;
        b1 = -2.0*(cosval + beta);
        b2 = 1.0 - alpha + beta;
        a0 = 1.0 + alpha;
        a1 = -2.0*cosval;
        a2 = 1.0 - alpha;
    }
        break;
        
    default:
        b0 = 0.0;
        b1 = 0.0;
        b2 = 0.0;
        a0 = 1.0;
        a1 = 0.0;
        a2 = 0.0;
        break;
}
#endif




//
//	first-order IIR filtering
//
//	x points to 0th input sample
//	y points to 0th output sample
//
//  may be in-place processing (y=x), but need not be
//
//	thisFilter must be initialized
//
void simpleFilterBlock(int32 numSamples, register float* y, register float* x, simpleFilter* thisFilter)
{
	register float	a1 = thisFilter->a1 - 1.0f;		// doing it this way might help with noise shaping
	register float	b0 = thisFilter->b0;
	register float	b1 = thisFilter->b1;
	register float	state = thisFilter->state;
	
	register int32	i = numSamples;
	
	while (--i >= 0)
	{
		register float _xx;
		_xx		= b1*state;
		state	+= a1*state + *x++;
		*y++	= _xx + b0*state;
	}
	
	thisFilter->state = state;
}


//
//	second-order IIR filtering
//
//	x points to 0th input sample
//	y points to 0th output sample
//
//  may be in-place processing (y=x), but need not be
//
//	thisFilter must be initialized
//
void biquadFilterBlock(int32 numSamples, register float* y, register float* x, biquadFilter* thisFilter)
{
	register float	a1 = thisFilter->a1;
	register float	a2 = thisFilter->a2;
	register float	b0 = thisFilter->b0;
	register float	b1 = thisFilter->b1;
	register float	b2 = thisFilter->b2;
	register float	state1 = thisFilter->state1;
	register float	state2 = thisFilter->state2;
	
	if (numSamples & 1)									// process 1 sample if numSamples is odd
	{
		register float _xx, _state2;
		_state2	= state2;
		state2	= state1;
		_xx		= b1*state1 + b2*_state2;
		state1	= a1*state1 + a2*_state2 + *x++;
		*y++	= _xx + b0*state1;
	}
	
	register int32	i = numSamples>>1;					// count is half of remaining samples
	
	while (--i >= 0)									// process samples in pairs to avoid unnecessary copying of states
	{
		register float _xx;
		_xx		= b1*state1 + b2*state2;
		state2	= a1*state1 + a2*state2 + *x++;			// swapping roles of state1 and state2
		*y++	= _xx + b0*state2;
		_xx		= b1*state2 + b2*state1;
		state1	= a1*state2 + a2*state1 + *x++;			// now swapping roles back
		*y++	= _xx + b0*state1;
	}
	
	thisFilter->state1 = state1;
	thisFilter->state2 = state2;
}


