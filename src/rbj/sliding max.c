/*
 *   An efficient sliding maximum algorithm that has computational cost that is O(log(L)).
 *
 *   Brookes: "Algorithms for Max and Min Filters with Improved Worst-Case Performance" 
 *   IEEE TRANSACTIONS ON CIRCUITS AND SYSTEMS—II: ANALOG AND DIGITAL SIGNAL PROCESSING, 
 *   VOL. 47, NO. 9, SEPTEMBER 2000
 */

 
#define A_REALLY_LARGE_NUMBER 3.40e38
    
typedef struct
    {
    unsigned long window_length;         // array_size/2 < window_length <= array_size
    unsigned long array_size;            // must be power of 2 for this simple implementation
    unsigned long input_index;           // the actual sample placement is at (array_size + input_index);
    float* big_array_base;               // the big array is malloc() separately and is actually twice array_size;
    } search_tree_array_data;


void initSearchArray(unsigned long window_length, search_tree_array_data* array_data)
    {
    array_data->window_length = window_length;
    
    array_data->array_size = 1;
    window_length--;
    while (window_length > 0)
        {
        array_data->array_size <<= 1;
        window_length >>= 1;
        }
    // array_size is a power of 2 such that
    // window_length <= array_size < 2*window_length
    // array_size = 2^ceil(log2(window_length)) = 2^(1+floor(log2(window_length-1)))
    
    array_data->input_index = 0;
    
    array_data->big_array_base = (float*)malloc(sizeof(float)*2*array_data->array_size);        // dunno what to do if malloc() fails.
    
    for (unsigned long n=0; n<2*array_data->array_size; n++)
        {
        array_data->big_array_base[n] = -A_REALLY_LARGE_NUMBER;        // init array.
        }                                                              // array_base[0] is never used.
    }



/*
 *   findMaxSample(value, &array_data) will place "value" into the circular
 *   buffer in the latter half of the array pointed to by array_data->big_array_base .
 *   it will then compare the value in "value" to its "sibling" value, takes the
 *   greater of the two and then pops up one generation to the parent node where 
 *   this parent also has a sibling and repeats the process.  since the other parent  
 *   nodes already have the max value of the two child nodes, when getting to the
 *   top-level parent node, this node will have the maximum value of all the samples
 *   in the big_array.  the number of iterations of this loop is ceil(log2(window_length)).
 */
    
float findMaxSample(float value, search_tree_array_data* array_data)
    {
    register float* big_array = array_data->big_array_base;
    
    register unsigned long index = array_data->array_size + array_data->input_index;        // our main buffer is in the latter half of the big array.
    
    while (index > 1UL)
        {
        big_array[index] = value;
    
        register float sibling_value = big_array[index ^ 1UL];        // toggle LSB, the upper bits of the sibling address are the same.
    
        if (value < sibling_value)
            {
            value = sibling_value;                        // use maximum of the two values
            }
    
        index >>= 1;                                     // parent address is index/2 (drop remainder or "sibling bit")
        }
    
    array_data->input_index++;
    if (array_data->input_index >= array_data->window_length)
        {
        array_data->input_index = 0;
        }
    
    return value;
    }

