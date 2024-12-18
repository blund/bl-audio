
#include "effect.h"
#include "reverb.h"

// -- reverb --
reverb_t* new_reverb(cadence_ctx* ctx) {
  reverb_t* r = ctx->alloc(sizeof(reverb_t));
  reverbInitialize(&r->rb);
  reverbSetParam(&r->rb, 44100, 0.5, 8.0, 4.0, 18000, 0.05);

  return r;
}

void set_reverb(cadence_ctx* ctx, reverb_t *r, float wet_percent, float time_s, float room_size_s,
		float cutoff_hz, float pre_delay_s) {
  reverbSetParam(&r->rb, 44100, wet_percent, time_s, room_size_s, cutoff_hz, pre_delay_s);
}

float apply_reverb(cadence_ctx* ctx, reverb_t* r, float sample) {
  if (r->chunk_idx < 32) {
    r->chunk[r->chunk_idx] = sample;
    r->chunk_idx++;
  }

  if (r->chunk_idx == 32) {
    r->chunk_idx = 0;
    Reverb(&r->rb, r->chunk, r->chunk);
  }

  return r->rb.left_output[r->chunk_idx]; // + r->rb.right_output[r->chunk_idx] ;
}
/*************************************************************
 *                                                            *
 *       Basic Jot Reverb                                     *
 *       in generic C code (floating-point version)           *
 *                                                            *
 *       Copyright (c)  1994-2012  Robert Bristow-Johnson     *
 *       All Rights Reserved                                  *
 *                                                            *
 *       Evaluation copy protected by prior non-disclosure    *
 *       agreement.  Commercial use permitted only by prior   *
 *       agreement.  Do not disclose any information in       *
 *       this file without permission from the copyright      *
 *       holder.                                              *
 *                                                            *
 *       rbj@audioimagination.com                             *
 *                                                            *
 *************************************************************/


/*
 *
 *   To use:
 *            Create or allocate memory for reverbBlock struct.
 *            Initialize reverbBlock struct with reverbInitialize().
 *            Set initial parameters of room with reverbSetParam().
 *            Process samples both left and right channels as CHUNKs with Reverb().
 *            Use reverbSetParam() every time a knob is turned and the reverb properties change.
 *            
 *   Every processing block (Delay, Filter, Reverb) owns its own output sample CHUNK.
 *
 */

#include <math.h>
#include "rbj-reverb.h"

#define MAX_REVERB_TIME			441000
#define MIN_REVERB_TIME			441
#define MAX_ROOM_SIZE			7552
#define MIN_ROOM_SIZE			(4*CHUNK_SIZE)
#define MAX_CUTOFF				0.4975
#define MIN_CUTOFF				0.1134

#define SQRT8					2.82842712474619	// sqrt(8)
#define ONE_OVER_SQRT8			0.353553390593274	//  1/sqrt(8)
#define ALPHA					0.943722057435498	//  pow(3/2, -1/(8-1))
													//       of the 8 delay lines, the longest is 3/2 times longer than the shortest.
													//       the longest delay is coupled to the room size.
													//       the delay lines then decrease exponentially in length.




void BuildPrimeTable(char* prime_number_table)
{
	int max_stride = (int)sqrt((float)PRIME_NUMBER_TABLE_SIZE);
	
	for(int i=0; i<PRIME_NUMBER_TABLE_SIZE; i++)
	{
		prime_number_table[i] = 1;									// initial value of all entries is 1
	}
	
	prime_number_table[0] = 0;										// now we zero out any entry that is not prime
	prime_number_table[1] = 0;
	int stride = 2;													// start with stride set to the smallest prime
	while (stride <= max_stride)
	{
		for(int i=2*stride; i<PRIME_NUMBER_TABLE_SIZE; i+=stride)	// start at the 2nd multiple of this prime, NOT the prime number itself!!!
		{
			prime_number_table[i] = 0;								// zero out table entries for all multiples of this prime number
		}
		stride++;
		while (prime_number_table[stride] == 0)						// go to next non-zero entry which is the next prime
		{
			stride++;
		}
	}
}


int FindNearestPrime(char* prime_number_table, int number)
{
	while (prime_number_table[number] == 0)
	{
		number--;
	}
	return number;
}





void reverbInitialize(reverbBlock* this_reverb)
{
	
	
	int	current_assigned_index = -CHUNK_SIZE;
	
	current_assigned_index	+= 119*CHUNK_SIZE;						// max number of samples (plus one extra CHUNK) allocated for this delay in this buffer
	this_reverb->left_predelay.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->left_predelay.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->left_predelay.input_index		= current_assigned_index;		// initial index must always be an integer multiple of CHUNK_SIZE
	this_reverb->left_predelay.delay_samples		= 882;							// let's start out with predelay about 20ms x 44.1 kHz
	
	current_assigned_index	+= 119*CHUNK_SIZE;
	this_reverb->right_predelay.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->right_predelay.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->right_predelay.input_index		= current_assigned_index;
	this_reverb->right_predelay.delay_samples	= 882;
	
	current_assigned_index	+= 119*CHUNK_SIZE;
	this_reverb->delay0.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay0.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay0.input_index		= current_assigned_index;
	this_reverb->delay0.delay_samples	= 1544;									// let's start out with room size about 35ms x 44.1 kHz
	
	current_assigned_index	+= 112*CHUNK_SIZE;
	this_reverb->delay1.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay1.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay1.input_index		= current_assigned_index;
	this_reverb->delay1.delay_samples	= 1457;
	
	current_assigned_index	+= 106*CHUNK_SIZE;
	this_reverb->delay2.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay2.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay2.input_index		= current_assigned_index;
	this_reverb->delay2.delay_samples	= 1375;
	
	current_assigned_index	+= 100*CHUNK_SIZE;
	this_reverb->delay3.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay3.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay3.input_index		= current_assigned_index;
	this_reverb->delay3.delay_samples	= 1297;
	
	current_assigned_index	+= 94*CHUNK_SIZE;
	this_reverb->delay4.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay4.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay4.input_index		= current_assigned_index;
	this_reverb->delay4.delay_samples	= 1224;
	
	current_assigned_index	+= 89*CHUNK_SIZE;
	this_reverb->delay5.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay5.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay5.input_index		= current_assigned_index;
	this_reverb->delay5.delay_samples	= 1155;
	
	current_assigned_index	+= 84*CHUNK_SIZE;
	this_reverb->delay6.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay6.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay6.input_index		= current_assigned_index;
	this_reverb->delay6.delay_samples	= 1090;
	
	current_assigned_index	+= 79*CHUNK_SIZE;
	this_reverb->delay7.buffer_base		= this_reverb->bigDelayBuffer;
	this_reverb->delay7.index_mask		= BIG_DELAY_BUFFER_SIZE-1;
	this_reverb->delay7.input_index		= current_assigned_index;
	this_reverb->delay7.delay_samples	= 1029;
	
	for(int i=0; i<BIG_DELAY_BUFFER_SIZE; i++)
	{
		this_reverb->bigDelayBuffer[i] = 0;
	}
	
	this_reverb->LPF0.a1 = -1.0;
	this_reverb->LPF0.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF0.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF0.output[i] = 0.0;
	
	this_reverb->LPF1.a1 = -1.0;
	this_reverb->LPF1.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF1.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF1.output[i] = 0.0;
	
	this_reverb->LPF2.a1 = -1.0;
	this_reverb->LPF2.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF2.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF2.output[i] = 0.0;
	
	this_reverb->LPF3.a1 = -1.0;
	this_reverb->LPF3.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF3.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF3.output[i] = 0.0;
	
	this_reverb->LPF4.a1 = -1.0;
	this_reverb->LPF4.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF4.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF4.output[i] = 0.0;
	
	this_reverb->LPF5.a1 = -1.0;
	this_reverb->LPF5.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF5.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF5.output[i] = 0.0;
	
	this_reverb->LPF6.a1 = -1.0;
	this_reverb->LPF6.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF6.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF6.output[i] = 0.0;
	
	this_reverb->LPF7.a1 = -1.0;
	this_reverb->LPF7.b0 = -ONE_OVER_SQRT8;
	this_reverb->LPF7.y1 = 0.0;
	for (int i=0; i<CHUNK_SIZE; i++)
		this_reverb->LPF7.output[i] = 0.0;
	
	this_reverb->left_reverb_state = 0.0;
	this_reverb->right_reverb_state = 0.0;
	
	BuildPrimeTable(this_reverb->primeNumberTable);
}


void reverbSetParam(reverbBlock* this_reverb, float fSampleRate, float fPercentWet, float fReverbTime, float fRoomSize, float fCutOffAbsorbsion, float fPreDelay)
{
	float wetCoef = fPercentWet/100.0;
	if (wetCoef > 1.0)
		wetCoef = 1.0;
	if (wetCoef < 0.0)
		wetCoef = 0.0;
	
	float fReverbTimeSamples = fReverbTime*fSampleRate;		// fReverbTime (expressed in seconds if fSampleRate is Hz) is the RT60 for the room
	if (fReverbTimeSamples > MAX_REVERB_TIME)
		fReverbTimeSamples = MAX_REVERB_TIME;
	if (fReverbTimeSamples < MIN_REVERB_TIME)
		fReverbTimeSamples = MIN_REVERB_TIME;
		
	float fRoomSizeSamples = fRoomSize*fSampleRate;			// fRoomSize is expressed in seconds = (room length)/(speed of sound)
	if (fRoomSizeSamples > MAX_ROOM_SIZE)
		fRoomSizeSamples = MAX_ROOM_SIZE;
	if (fRoomSizeSamples < MIN_ROOM_SIZE)
		fRoomSizeSamples = MIN_ROOM_SIZE;
	
	float fCutOff = fCutOffAbsorbsion/fSampleRate;
	if (fCutOff > MAX_CUTOFF)
		fCutOff = MAX_CUTOFF;
	if (fCutOff < MIN_CUTOFF)
		fCutOff = MIN_CUTOFF;

	float fPreDelaySamples = fPreDelay*fSampleRate;			// fPreDelay is expressed in seconds if fSampleRate is Hz
	if (fPreDelaySamples > MAX_ROOM_SIZE)
		fPreDelaySamples = MAX_ROOM_SIZE;
	if (fPreDelaySamples < 0.0)
		fPreDelaySamples = 0.0;
	
	
	float	fCutoffCoef		= exp(-6.28318530717959*fCutOff);
	
	this_reverb->dry_coef	= 1.0 - wetCoef;
	
	wetCoef	*= SQRT8 * (1.0 - exp(-13.8155105579643*fRoomSizeSamples/fReverbTimeSamples));			// additional attenuation for small room and long reverb time  <--  exp(-13.8155105579643) = 10^(-60dB/10dB)
																									//  toss in whatever fudge factor you need here to make the reverb louder
	this_reverb->wet_coef0	= wetCoef;
	this_reverb->wet_coef1	= -fCutoffCoef*wetCoef;
	
	fCutoffCoef /=  (float)FindNearestPrime(this_reverb->primeNumberTable, (int)fRoomSizeSamples);
	
	float	fDelaySamples	= fRoomSizeSamples;
	float	beta			= -6.90775527898214/fReverbTimeSamples;			// 6.90775527898214 = log(10^(60dB/20dB))  <-- fReverbTime is RT60
	float	f_prime_value;
	int		prime_value;
	
	this_reverb->left_predelay.delay_samples = (int)fPreDelaySamples;
	this_reverb->right_predelay.delay_samples = (int)fPreDelaySamples;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay0.delay_samples	= prime_value - CHUNK_SIZE;									// we subtract 1 CHUNK of delay, because this signal feeds back, causing an extra CHUNK delay
	f_prime_value			= (float)prime_value;
	this_reverb->LPF0.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF0.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF0.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay1.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF1.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF1.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF1.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay2.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF2.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF2.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF2.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay3.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF3.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF3.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF3.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay4.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF4.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF4.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF4.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay5.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF5.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF5.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF5.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay6.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF6.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF6.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF6.a1);
	fDelaySamples *= ALPHA;
	
	prime_value				= FindNearestPrime(this_reverb->primeNumberTable, (int)fDelaySamples);
	this_reverb->delay7.delay_samples	= prime_value - CHUNK_SIZE;
	f_prime_value			= (float)prime_value;
	this_reverb->LPF7.a1	= f_prime_value*fCutoffCoef - 1.0;
	this_reverb->LPF7.b0	= ONE_OVER_SQRT8*exp(beta*f_prime_value)*(this_reverb->LPF7.a1);
}




void Delay(delayBlock* this_delay, float* input)
{
	register float*		output		= &(this_delay->output[0]);
	register float*		delay_ptr	= this_delay->buffer_base;
	register int		index_mask	= this_delay->index_mask;
	register int		index		= this_delay->input_index;

	delay_ptr += index;
	
	for(register int i=CHUNK_SIZE; i>0; i--)
	{
		*delay_ptr++ = *input++;						// no wrapping nor masking necessary because input index should always start as a multiple of CHUNK_SIZE
	}
	
	index -=  this_delay->delay_samples;				// go to first delayed sample
	delay_ptr = this_delay->buffer_base;
	
	for(register int i=CHUNK_SIZE; i>0; i--)
	{
		index &= index_mask;							// must wrap index and 
		*output++ = delay_ptr[index++];					//                     reference from buffer base every sample
	}
	
	index = this_delay->input_index;
	index += CHUNK_SIZE;								// advance input index to pick up where we left off
	index &= index_mask;								// this might need to wrap
	this_delay->input_index = index;					// save state
}


//
//	transfer function:			H(z) = b0/(1 - (a1+1)*z^(-1))
//
//	for the nth delay line:		a1   = delay[n]/size * exp(-2*pi*fcutoff/Fs) - 1  =   pole - 1
//								b0   = a1/sqrt(8) * 10^(-(60dB*delay[n]/RT60)/20dB)
//
void Filter(filterBlock* this_filter, float* input)
{
	register float*	output	= &(this_filter->output[0]);

	register float	b0 = this_filter->b0;				// feedforward coefficient
	register float	a1 = this_filter->a1;				// feedback coefficient
	
	register float 	y1 = this_filter->y1;				// previous output

	register float	output_sample = y1;					// now is previous output sample, y[n-1]
	
	for (register int i=CHUNK_SIZE; i>0; i--)
	{
		y1 += b0 * (*input++);							// y[n-1] + b0*x[n]
		y1 += a1 * output_sample;						// y[n-1] + b0*x[n] + a1*y[n-1]
		output_sample = y1;
		*output++ = output_sample;
	}
	
	this_filter->y1 = y1;								// save state
}


#define BEGIN_ROW(source_signal)										\
						input_ptr = source_signal;						\
						for(register int i=0; i<CHUNK_SIZE; i++)		\
						{												\
							register float	acc = *input_ptr++;

#define	PLUS_ONE(in_ptr)	acc += *in_ptr++;
#define	MINUS_ONE(in_ptr)	acc -= *in_ptr++;

#define	END_ROW(node)													\
							node[i] = acc;								\
						}												\
						x0 -= CHUNK_SIZE;								\
						x1 -= CHUNK_SIZE;								\
						x2 -= CHUNK_SIZE;								\
						x3 -= CHUNK_SIZE;								\
						x4 -= CHUNK_SIZE;								\
						x5 -= CHUNK_SIZE;								\
						x6 -= CHUNK_SIZE;								\
						x7 -= CHUNK_SIZE;


void Reverb(reverbBlock* this_reverb, float* left_input, float* right_input)
{
	Delay(&(this_reverb->left_predelay), left_input);
	Delay(&(this_reverb->right_predelay), right_input);
	
	register float*	x0 = &(this_reverb->LPF0.output[0]);
	register float*	x1 = &(this_reverb->LPF1.output[0]);
	register float*	x2 = &(this_reverb->LPF2.output[0]);
	register float*	x3 = &(this_reverb->LPF3.output[0]);
	register float*	x4 = &(this_reverb->LPF4.output[0]);
	register float*	x5 = &(this_reverb->LPF5.output[0]);
	register float*	x6 = &(this_reverb->LPF6.output[0]);
	register float*	x7 = &(this_reverb->LPF7.output[0]);
	
	register float*	input_ptr = 0;							// needed for macro expansions below
	
	BEGIN_ROW(&(this_reverb->left_predelay.output[0]))
	PLUS_ONE(x0)
	PLUS_ONE(x1)
	PLUS_ONE(x2)
	PLUS_ONE(x3)
	MINUS_ONE(x4)
	MINUS_ONE(x5)
	MINUS_ONE(x6)
	MINUS_ONE(x7)
	END_ROW(this_reverb->node0)
	
	BEGIN_ROW(&(this_reverb->right_predelay.output[0]))
	PLUS_ONE(x0)
	PLUS_ONE(x1)
	MINUS_ONE(x2)
	MINUS_ONE(x3)
	PLUS_ONE(x4)
	PLUS_ONE(x5)
	MINUS_ONE(x6)
	MINUS_ONE(x7)
	END_ROW(this_reverb->node1)
	
	BEGIN_ROW(&(this_reverb->right_predelay.output[0]))
	PLUS_ONE(x0)
	PLUS_ONE(x1)
	MINUS_ONE(x2)
	MINUS_ONE(x3)
	MINUS_ONE(x4)
	MINUS_ONE(x5)
	PLUS_ONE(x6)
	PLUS_ONE(x7)
	END_ROW(this_reverb->node2)
	
	BEGIN_ROW(&(this_reverb->left_predelay.output[0]))
	PLUS_ONE(x0)
	MINUS_ONE(x1)
	PLUS_ONE(x2)
	MINUS_ONE(x3)
	PLUS_ONE(x4)
	MINUS_ONE(x5)
	PLUS_ONE(x6)
	MINUS_ONE(x7)
	END_ROW(this_reverb->node3)
	
	BEGIN_ROW(&(this_reverb->right_predelay.output[0]))
	PLUS_ONE(x0)
	MINUS_ONE(x1)
	PLUS_ONE(x2)
	MINUS_ONE(x3)
	MINUS_ONE(x4)
	PLUS_ONE(x5)
	MINUS_ONE(x6)
	PLUS_ONE(x7)
	END_ROW(this_reverb->node4)
	
	BEGIN_ROW(&(this_reverb->left_predelay.output[0]))
	PLUS_ONE(x0)
	MINUS_ONE(x1)
	MINUS_ONE(x2)
	PLUS_ONE(x3)
	PLUS_ONE(x4)
	MINUS_ONE(x5)
	MINUS_ONE(x6)
	PLUS_ONE(x7)
	END_ROW(this_reverb->node5)
	
	BEGIN_ROW(&(this_reverb->left_predelay.output[0]))
	PLUS_ONE(x0)
	MINUS_ONE(x1)
	MINUS_ONE(x2)
	PLUS_ONE(x3)
	MINUS_ONE(x4)
	PLUS_ONE(x5)
	PLUS_ONE(x6)
	MINUS_ONE(x7)
	END_ROW(this_reverb->node6)
	
	BEGIN_ROW(&(this_reverb->right_predelay.output[0]))
	PLUS_ONE(x0)
	PLUS_ONE(x1)
	PLUS_ONE(x2)
	PLUS_ONE(x3)
	PLUS_ONE(x4)
	PLUS_ONE(x5)
	PLUS_ONE(x6)
	PLUS_ONE(x7)
	END_ROW(this_reverb->node7)

	
	register float*	input = left_input;
	register float*	output = &(this_reverb->left_output[0]);
	register float	reverb_output_state = this_reverb->left_reverb_state;
	for (register int i=CHUNK_SIZE; i>0; i--)
	{
		register float reverb_output = *(x0++) + *(x2++) + *(x4++) + *(x6++);
		register float output_acc = this_reverb->dry_coef * (*input++);
		output_acc += this_reverb->wet_coef0 * reverb_output;
		output_acc += this_reverb->wet_coef1 * reverb_output_state;
		*output++ = output_acc;
		reverb_output_state = reverb_output;
	}
	this_reverb->left_reverb_state = reverb_output_state;
	
	
	input = right_input;
	output = &(this_reverb->right_output[0]);
	reverb_output_state = this_reverb->right_reverb_state;
	for (register int i=CHUNK_SIZE; i>0; i--)
	{
		register float reverb_output = *(x1++) + *(x3++) + *(x5++) + *(x7++);
		register float output_acc = this_reverb->dry_coef * (*input++);
		output_acc += this_reverb->wet_coef0 * reverb_output;
		output_acc += this_reverb->wet_coef1 * reverb_output_state;
		*output++ = output_acc;
		reverb_output_state = reverb_output;
	}
	this_reverb->right_reverb_state = reverb_output_state;
	
	
	Delay(&(this_reverb->delay0), this_reverb->node0);
	Delay(&(this_reverb->delay1), this_reverb->node1);
	Delay(&(this_reverb->delay2), this_reverb->node2);
	Delay(&(this_reverb->delay3), this_reverb->node3);
	Delay(&(this_reverb->delay4), this_reverb->node4);
	Delay(&(this_reverb->delay5), this_reverb->node5);
	Delay(&(this_reverb->delay6), this_reverb->node6);
	Delay(&(this_reverb->delay7), this_reverb->node7);
	
	Filter(&(this_reverb->LPF0), &(this_reverb->delay0.output[0]));
	Filter(&(this_reverb->LPF1), &(this_reverb->delay1.output[0]));
	Filter(&(this_reverb->LPF2), &(this_reverb->delay2.output[0]));
	Filter(&(this_reverb->LPF3), &(this_reverb->delay3.output[0]));
	Filter(&(this_reverb->LPF4), &(this_reverb->delay4.output[0]));
	Filter(&(this_reverb->LPF5), &(this_reverb->delay5.output[0]));
	Filter(&(this_reverb->LPF6), &(this_reverb->delay6.output[0]));
	Filter(&(this_reverb->LPF7), &(this_reverb->delay7.output[0]));
}




/*****************************************************************************************************************************************

   Algorithm Description:


   The core of the reverb algorithm implemented here is contained in
   a feedback "matrix" and a set of eight delay lines. This structure
   represents a generalized feedback network in which each delay line
   input receives a linear combination of each of the delay outputs and
   of the input signal to the reverberator.  It is based on the published
   work of Jot:
 
      Digital Delay Networks for Designing Artificial Reverberators
 
      Within the framework of Schroeder's parallel comb filter reverberator,
      a method is proposed for controlling the decay characteristics (avoiding
      unnatural resonances) and for compensating the frequency response. The
      method is extended to any recursive delay network having a unitary
      feedback matrix, and allows selection of the reverberator structure
      irrespective of reverberation time control.
 
      Author: Jot, Jean-Marc
      Affiliation: Antoine Chaigne, Enst, departement SIGNAL, Paris, France
      AES Convention: 90 (February 1991)   Preprint Number:3030
 
   Mathematically, this can be represented as a matrix multiply operation,
   where the outputs of the delay lines are a column vector. The inputs to the
   delay line are the sum of the input signal and the result of multiplying the
   feedback matrix by the column vector of delay outputs.

   As might be seen by looking at the block diagram, this structure
   has an advantage over "classical" reverb structures in that it can provide
   an exponentially increasing "time density" of echoes.  For the matrix shown
   below, a single pulse to the input will result in eight pulses at the output,
   followed by 8^2 pulses, 8^3, etc. The tricky part is guaranteeing stability
   of the delay network.  To test this, try altering
   several values in the matrix initialization section of this code.

   There are several ways of guaranteeing stability to a feedback network.  A
   sufficient condition is that the feedback matrix is "orthogonal". There are
   many possible orthogonal matrices.  However, not all result in good-sounding
   reverbs.

   In addition to having a stable feedback network, frequency independent and
   frequency dependent attenuation factors must be added to simulate the
   absorption of air and the inverse square law attenuation of sound. Both
   types of absorption are applied at the output of each of the eight delay
   lines, shown as LPF0 to LPF7. The frequency-independant absorption,
   controls the overall decay time of the reverb. The frequency-dependant
   absorption is implemented as simple one-pole lowpass filters at the output of
   each delay. (Actually, both factors are incorporated into the same one-pole
   filter for efficient computation. They are shown as seperate on the block diagram
   for clarity.) The only tricky part in the implementation of the absorption factors
   is that the absorption for each delay line is made proportional to the delay
   length. This is so that all poles will have roughly equal magnitude. Otherwise,
   the longer delays would "ring-out" longer in the reverberant decay.

   The delay lengths are chosen to fall over a 1:1.5 range and are made to be prime
   numbers so as to avoid any superposition of resonances. The delay lengths are
   typically chosen to suggest an apparent room size, as they represent a kind of
   mean-free echo path in the room being simulated. However, the reverberant quality
   will generally be better at the highest delay settings. This is because the
   "modal density" (the number of resonant modes), is proportional to the total
   amount of delay in the reverb algorithm. With too low a modal density, distinct
   ringing modes will be heard in the reverberant sound.

   To create a stereo output, the outputs of each of the delay lines are summed
   and panned. For maximum stereo effect, alternate delay lines are panned hard
   left and right. A "stereo width" parameter might be implemented by making
   this variable.

   After the summing, a tone compensation filter is implemented on each channel
   output. This is necessary to keep the reverb frequency response relatively
   flat. It is implemented as a one-zero filter using the same coefficient as
   the high-frequency absorption one-pole filter.

   Finally, the output is mixed with the dry signal to allow control over the
   amount of reverberation.


   Implementation Notes:


   The delay lines are implemented using a base pointer and offset method. With
   this method, the entire delay memory is used to implement one large delay buffer,
   or circular queue. The write pointer is incremented each sample period (modulo
   the total delay length) and all writes and read are done at a fixed "offset"
   from the base pointer. This is analogous to having a large tape loop with
   multiple read and write heads. 

   The audio data processing in this algorithm is accomplished in 64-sample "CHUNKS".
   This makes the delay line access much more efficient. Essentially, various pointer
   and coefficient loads can be amortized over a multi-sample period. There is no reason
   why the processing could not be done in even larger chunks, as long as there is
   enough memory available (signal storage is proportional to CHUNK length.)





   Block Diagram:
 

                                             Unitary-Gain Feedback Matrix
 
                                               -----------------------
                      --------<---------------|+1 +1 +1 +1 -1 -1 -1 -1|<-----------------
                     |                        |                       |                  |
                     |   -------<-------------|+1 +1 -1 -1 +1 +1 -1 -1|<---------------  |
                     |  |                     |                       |                | |
                     |  |   ------<-----------|+1 +1 -1 -1 -1 -1 +1 +1|<-------------  | |
                     |  |  |                  |                       |              | | |
                     |  |  |   -----<---------|+1 -1 +1 -1 +1 -1 +1 -1|<-----------  | | |
                     |  |  |  |               |                       |            | | | |
                     |  |  |  |   ----<-------|+1 -1 +1 -1 -1 +1 -1 +1|<---------  | | | |
                     |  |  |  |  |            |                       |          | | | | |
                     |  |  |  |  |   ---<-----|+1 -1 -1 +1 +1 -1 -1 +1|<-------  | | | | |
                     |  |  |  |  |  |         |                       |        | | | | | |
                     |  |  |  |  |  |   --<---|+1 -1 -1 +1 -1 +1 +1 -1|<-----  | | | | | |
                     |  |  |  |  |  |  |      |                       |      | | | | | | |
                     |  |  |  |  |  |  |   -<-|+1 +1 +1 +1 +1 +1 +1 +1|<---  | | | | | | |
                     |  |  |  |  |  |  |  |    -----------------------     | | | | | | | |
                     |  |  |  |  |  |  |  |                                | | | | | | | |
                     |  |  |  |  |  |  |  |                                | | | | | | | |
                     |  |  |  |  |  |  |  |                                | | | | | | | |
                     |  |  |  |  |  |  |  |                                | | | | | | | |
                     |  |  |  |  |  |  |  |                                | | | | | | | |
                     v  |  |  |  |  |  |  |                                | | | | | | | |
           -------->(+)-|--|--|--|--|--|--|---->[ delay0 ]---->[ LPF0 ]----*-|-|-|-|-|-|-|---(wet)------------------------->(+)-------> L
          |             v  |  |  |  |  |  |                                  | | | | | | |                                   ^
          |     ------>(+)-|--|--|--|--|--|---->[ delay1 ]---->[ LPF1 ]------*-|-|-|-|-|-|---(wet)--------------->(+)--------|--------> R
          |    |           v  |  |  |  |  |                                    | | | | | |                         ^         |
          |    *--------->(+)-|--|--|--|--|---->[ delay2 ]---->[ LPF2 ]--------*-|-|-|-|-|---(wet)-----------------|--->(+)--'
          |    |              v  |  |  |  |                                      | | | | |                         |     ^
          *----------------->(+)-|--|--|--|---->[ delay3 ]---->[ LPF3 ]----------*-|-|-|-|---(wet)----------->(+)--'     |
          |    |                 v  |  |  |                                        | | | |                     ^         |
          |    *--------------->(+)-|--|--|---->[ delay4 ]---->[ LPF4 ]------------*-|-|-|---(wet)-------------|--->(+)--'
          |    |                    v  |  |                                          | | |                     |     ^
          *----------------------->(+)-|--|---->[ delay5 ]---->[ LPF5 ]--------------*-|-|---(wet)------->(+)--'     |
          |    |                       v  |                                            | |                 ^         |
          *-------------------------->(+)-|---->[ delay6 ]---->[ LPF6 ]----------------*-|---(wet)---------|--->(+)--'
          |    |                          v                                              |                 |     ^
          |    *------------------------>(+)--->[ delay7 ]---->[ LPF7 ]------------------*---(wet)--->(+)--'     |
          |    |                                                                                       ^         |
       [ pre-delay ]                                                                                   |         |
          ^    ^                                                                                       |         |
          |    |                                                                                       |         |
  R  >----|----*---------------------------------------------------------------------------(1-wet)-----'         |
          |                                                                                                      |
  L  >----*--------------------------------------------------------------------------------(1-wet)---------------'


*****************************************************************************************************************************************/


