#ifndef _WEIGHTED_SAMPLING_TREE_H
#define _WEIGHTED_SAMPLING_TREE_H

#include <stddef.h>



typedef struct WeightedSamplingTree WeightedSamplingTree;



// May return NULL in case of allocation failure.
// weights and values must have the length of size.
// Does not take ownership of the array weights, but it must not
// be changed from the outside after calling this function until wst is freed again.
// Result pointer must be freed using wst_free(...)
WeightedSamplingTree* wst_new(double* weights, size_t size);

void wst_free(WeightedSamplingTree* wst);



// change the weight assigned to an index
void wst_change_weight(WeightedSamplingTree* wst, size_t index, double new_weight);



// returns random index
// does not remove the selected element
size_t wst_sample_with_replacement(const WeightedSamplingTree* wst);



// changes value in weights array
size_t wst_sample_without_replacement(WeightedSamplingTree* wst);



#endif
