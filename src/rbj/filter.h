//
//  filter.h
//

#ifndef filter_h
#define Zfilter_h

#define PI				(3.1415926535897932384626433832795028841972f)		/* pi */
#define ONE_OVER_PI		(0.3183098861837906661338147750939f)
#define TWOPI			(6.2831853071795864769252867665590057683943f)		/* 2*pi */
#define ONE_OVER_TWOPI	(0.15915494309189535682609381638f)
#define PI_2			(1.5707963267948966192313216916397514420986f)		/* pi/2 */
#define TWO_OVER_PI		(0.636619772367581332267629550188f)
#define LN2				(0.6931471805599453094172321214581765680755f)		/* ln(2) */
#define ONE_OVER_LN2	(1.44269504088896333066907387547f)
#define LN10			(2.3025850929940456840179914546843642076011f)		/* ln(10) */
#define ONE_OVER_LN10	(0.43429448190325177635683940025f)
#define ROOT2			(1.4142135623730950488016887242096980785697f)		/* sqrt(2) */
#define ONE_OVER_ROOT2	(0.707106781186547438494264988549f)

#define DB_LOG2_ENERGY			(3.01029995663981154631945610163f)			/* dB = DB_LOG2_ENERGY*log2(energy) */
#define DB_LOG2_AMPL			(6.02059991327962309263891220326f)			/* dB = DB_LOG2_AMPL*log2(amplitude) */
#define ONE_OVER_DB_LOG2_AMPL	(0.16609640474436811218256075335f)			/* amplitude = exp2(ONE_OVER_DB_LOG2_AMPL*dB) */

#define DB_LOG_ENERGY			(4.342944819032518f)			/* dB = DB_LOG_ENERGY*log(energy) */
#define DB_LOG_AMPL				(8.685889638065037f)			/* dB = DB_LOG_AMPL*log(amplitude) */
#define ONE_OVER_DB_LOG_AMPL	(0.115129254649702f)			/* amplitude = exp(ONE_OVER_DB_LOG_AMPL*dB) */


enum EQ_type
{
	LPF,
	HPF,
	BPF,
	BPF0,
	notch,
	APF,
	peakingEQ,
	lowShelf,
	highShelf,
	lowShelf1,
	highShelf1,
};

typedef struct
{
	float Fs;
	enum EQ_type type;
	float f0;
	float dBgain;
	float Q;
	float filterCoefficients[5];	/* b0, b1, b2, a1, a2   (a0 is normalized to 1) */
	float graphCoefficients[6];		/* B2, B1, B0, A2, A1, A0 */
} sectionData;


void computeSectionCofficients(float Fs, enum EQ_type type, float f0, float dBgain, float Q, sectionData* data);
void plotSectionFrequencyResponse(int32 Npoints, float Fmin, float Fmax, float* frequencyValues, float* dBvalues, sectionData* data);
void filterBuffer(int32 Nsamples, int32 Nsections, float* y, float* x, float* filterStates, sectionData** sections);


typedef	struct
{
    float	a1;
    float	b0;
    float	b1;
    float	state;
} simpleFilter;


typedef	struct
{
    float	a1;
    float	a2;
    float	b0;
    float	b1;
    float	b2;
    float	state1;
    float	state2;
} biquadFilter;

void simpleFilterBlock(int32 numSamples, float* y, float* x, simpleFilter* thisFilter);
void biquadFilterBlock(int32 numSamples, float* y, float* x, biquadFilter* thisFilter);


#endif
