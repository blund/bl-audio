#include <stdlib.h>
#include <stdio.h>

int decimal_to_binary(char *dec, long *bin)		// returns an error code, 0 if no error, -1 if too big, -2 for other formatting errors
	{
	int i = 0;
	
	int past_leading_space = 0;
	while (i <= 64 && !past_leading_space)		// first get past leading spaces
		{
		if (dec[i] == ' ')
			{
			i++;
			}
		 else
			{
			past_leading_space = 1;
			}
		}
	if (!past_leading_space)
		{
		return -2;								// 64 leading spaces does not a number make
		}
												// at this point the only legitimate remaining chars are decimal digits or a leading plus or minus sign
	int negative = 0;
	if (dec[i] == '-')
		{
		negative = 1;
		i++;
		}
	 else if (dec[i] == '+')
		{
		i++;									// do nothing but go on to next char
		}
												// now the only legitimate chars are decimal digits
	if (dec[i] == '\0')
		{
		return -2;								// there needs to be at least one good digit before terminating string
		}
	
	unsigned long abs_bin = 0;
	while (i <= 64 && dec[i] != '\0')
		{
		if ( dec[i] >= '0' && dec[i] <= '9' )
			{
			if (abs_bin > 214748364)
				{
				return -1;								// this is going to be too big
				}
			abs_bin <<= 1;
			abs_bin += (abs_bin<<2);					// an efficient way to multiply by 10
//			abs_bin *= 10;								// previous value gets bumped to the left one digit...				
			abs_bin += (unsigned long)(dec[i] - '0');	// ... and a new digit appended to the right
			i++;
			}
		 else
			{
			return -2;									// not a legit digit in text string
			}
		}
	
	if (dec[i] != '\0')
		{
		return -2;								// not terminated string in 64 chars
		}
	
	if (negative)
		{
		if (abs_bin > 2147483648)
			{
			return -1;							// too big
			}
		*bin = -(long)abs_bin;
		}
	 else
		{
		if (abs_bin > 2147483647)
			{
			return -1;							// too big
			}
		*bin = (long)abs_bin;
		}
	
	return 0;
	}


void binary_to_decimal(char *dec, long bin)
	{
	unsigned long long acc;				// 64-bit unsigned integer
	
	if (bin < 0)
		{
		*(dec++) = '-';					// leading minus sign
		bin = -bin;						// make bin value positive
		}
	
	acc = 989312855LL*(unsigned long)bin;		// very nearly 0.2303423488 * 2^32
	acc += 0x00000000FFFFFFFFLL;				// we need to round up
	acc >>= 32;
	acc += 57646075LL*(unsigned long)bin;		// (2^59)/(10^10)  =  57646075.2303423488  =  57646075 + (989312854.979825)/(2^32)  
	
	int past_leading_zeros = 0;
	for (int i=9; i>=0; i--)			// maximum number of digits is 10
		{
		acc <<= 1;
		acc += (acc<<2);				// an efficient way to multiply by 10
//		acc *= 10;
		
		unsigned int digit = (unsigned int)(acc >> 59);		// the digit we want is in bits 59 - 62
		
		if (digit > 0)
			{
			past_leading_zeros = 1;
			}
		
		if (past_leading_zeros)
			{
			*(dec++) = '0' + digit;
			}
		
//		printf(" i = %d, acc = 0x%016llx \n", i, acc<<1);	// put digit into upper 4 bits to be easily read
		
		acc &= 0x07FFFFFFFFFFFFFFLL;	// mask off this digit and go on to the next digit
		}
	
	if (!past_leading_zeros)			// if all digits are zero ...
		{
		*(dec++) = '0';					// ... put in at least one zero digit
		}
	
	*dec = '\0';						// terminate string
	}


#if 1

int main (int argc, const char* argv[])
	{
	char dec[64];
	long bin, result1, result2;
	unsigned long num_errors;
	long long long_long_bin;
	
	num_errors = 0;
	for (long_long_bin=-2147483648LL; long_long_bin<=2147483647LL; long_long_bin++)
		{
		bin = (long)long_long_bin;
		if ((bin&0x00FFFFFFL) == 0)
			{
			printf("bin = %ld \n", bin);		// this is to tell us that things are moving along
			}
		binary_to_decimal(dec, bin);
		decimal_to_binary(dec, &result1);
		sscanf(dec, "%ld", &result2);			// decimal_to_binary() should do the same as this sscanf()
		
		if (bin != result1 || bin != result2)
			{
			num_errors++;
			printf("bin = %ld, result1 = %ld, result2 = %ld, num_errors = %ld, dec = %s \n", bin, result1, result2, num_errors, dec);
			}
		}
	
	printf("num_errors = %ld \n", num_errors);
	
	return 0;
	}

#else

int main (int argc, const char* argv[])
	{
	char dec[64];
	long bin;
	
	printf("bin = ");
	scanf("%ld", &bin);
	while (bin != 0)
		{
		binary_to_decimal(dec, bin);
		printf("dec = %s \n", dec);
		printf("bin = ");
		scanf("%ld", &bin);
		}
	
	return 0;
	}

#endif
