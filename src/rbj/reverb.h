
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


#ifndef REVERB_H
#define REVERB_H


#define PRIME_NUMBER_TABLE_SIZE	7600

void BuildPrimeTable(char* prime_number_table);
int FindNearestPrime(char* prime_number_table, int number);


#define CHUNK_SIZE				64					// must be power of 2
#define BIG_DELAY_BUFFER_SIZE	65536				// must be power of 2

typedef struct {
	float		output[CHUNK_SIZE];					// each processing block owns its own output CHUNK
	
	float*		buffer_base;						// set to &(buffer[0])
	int			index_mask;							// set to sizeof(buffer) - 1.  sizeof(buffer) must be power of 2
	int			input_index;						// points to where in buffer samples go in
	int			delay_samples;						// the delay amount in samples
} delayBlock;


typedef struct {
	float		output[CHUNK_SIZE];					// each processing block owns its own output CHUNK

	float		b0;									// forward gain coefficient
	float		a1;									// feedback coefficient minus 1
	float		y1;									// feedback state
} filterBlock;


typedef struct {
	float		left_output[CHUNK_SIZE];			// each processing block owns its own output CHUNK
	float		right_output[CHUNK_SIZE];

	float			bigDelayBuffer[BIG_DELAY_BUFFER_SIZE];
	char			primeNumberTable[PRIME_NUMBER_TABLE_SIZE];
		
	float			dry_coef;
	float			wet_coef0;
	float			wet_coef1;
	float			left_reverb_state;
	float			right_reverb_state;
	
	float			node0[CHUNK_SIZE];
	float			node1[CHUNK_SIZE];
	float			node2[CHUNK_SIZE];
	float			node3[CHUNK_SIZE];
	float			node4[CHUNK_SIZE];
	float			node5[CHUNK_SIZE];
	float			node6[CHUNK_SIZE];
	float			node7[CHUNK_SIZE];
	
	delayBlock		left_predelay;
	delayBlock		right_predelay;
	
	delayBlock		delay0;
	delayBlock		delay1;
	delayBlock		delay2;
	delayBlock		delay3;
	delayBlock		delay4;
	delayBlock		delay5;
	delayBlock		delay6;
	delayBlock		delay7;
	
	filterBlock		LPF0;
	filterBlock		LPF1;
	filterBlock		LPF2;
	filterBlock		LPF3;
	filterBlock		LPF4;
	filterBlock		LPF5;
	filterBlock		LPF6;
	filterBlock		LPF7;
} reverbBlock;


void reverbInitialize(reverbBlock* this_reverb);
void reverbSetParam(reverbBlock* this_reverb, float fSampleRate, float fPercentWet, float fReverbTime, float fRoomSize, float fCutOffAbsorbsion, float fPreDelay);
void Delay(delayBlock* this_delay, float* input);
void Filter(filterBlock* this_filter, float* input);
void Reverb(reverbBlock* this_reverb, float* left_input, float* right_input);

#endif

