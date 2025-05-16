#include "weighted_sampling_tree.h"

#include <assert.h>
#include <stdint.h>
#include <stdlib.h>

#include <stdio.h> // DEBUG



struct WeightedSamplingTree {
    double* tree;
    double* weights;
    size_t size;
    size_t internal_size; // the true size, which has to be one less than a power of two
};



// must not be called on the root node
static inline size_t _tree_parent(const size_t index)
{
    return (index - 1) / 2;
}

static inline size_t _tree_lchild(const size_t index)
{
    return 2 * index + 1;
}

static inline size_t _tree_rchild(const size_t index)
{
    return 2 * index + 2;
}

static inline size_t _tree_idx_to_weights_idx(const WeightedSamplingTree* wst, const size_t tree_index)
{
    return tree_index - wst->internal_size;
}

static inline size_t _weights_parent(const WeightedSamplingTree* wst, const size_t weights_index)
{
    return _tree_parent(weights_index + wst->internal_size);
}



// returns the smallest power of two that is equal to or greater than x
static size_t _next_power_of_2(size_t x)
{
    assert(x < ((UINT32_MAX >> 1) + 1));
    size_t p2 = 1;
    while(p2 < x) {
        p2 = p2 << 1;
        assert(p2 > 0);
    }
    return p2;
}



static void _init_tree(WeightedSamplingTree* wst)
{
    assert(wst != NULL && wst->tree != NULL && wst->weights != NULL);
    assert(wst->internal_size >= wst->size && wst->size >= 3);
    assert(((wst->internal_size + 1) & wst->internal_size) == 0); // assert wst->internal_size + 1 is a power of two
    // step 1: set the values of the last level of internal nodes
    size_t start_node = _weights_parent(wst, wst->size - 1);
    if(wst->size % 2 == 1) { // edge case: odd number ==> last leaf has no sibling, trying to access it could result in segfault
        wst->tree[_weights_parent(wst, wst->size - 1)] = wst->weights[wst->size - 1];
        assert(start_node - 1 == _weights_parent(wst, wst->size - 2));
        start_node--;
    }
    size_t end_node = _weights_parent(wst, 0);
    for(size_t node = start_node; node >= end_node; node--) {
        wst->tree[node] = wst->weights[_tree_idx_to_weights_idx(wst, _tree_lchild(node))] +
                          wst->weights[_tree_idx_to_weights_idx(wst, _tree_rchild(node))];
    }
    // step 2: set values of the higher levels of internal nodes
    start_node = _tree_parent(_weights_parent(wst, wst->size - 1));
    for(size_t node = start_node; node > 0; node--) { // can't include root (0) here because of underflow
        wst->tree[node] = wst->tree[_tree_lchild(node)] + wst->tree[_tree_rchild(node)];
    }
    wst->tree[0] = wst->tree[_tree_lchild(0)] + wst->tree[_tree_rchild(0)];
}



// May return NULL in case of allocation failure.
// weights and values must have the length of size.
// Does not take ownership of the array weights, but it must not
// be changed from the outside after calling this function until wst is freed again.
// Result pointer must be freed using wst_free(...)
WeightedSamplingTree* wst_new(double* weights, size_t size)
{
    assert(size >= 3);
    assert(weights != NULL);
    WeightedSamplingTree* wst = malloc(sizeof(WeightedSamplingTree));
    if(!wst) {
        return NULL;
    }
    wst->size = size;
    wst->internal_size = _next_power_of_2(size) - 1;
    wst->weights = weights;
    wst->tree = calloc(wst->internal_size, sizeof(double)); // zero-bytes are the bit representation of the value +0 for IEEE 754 doubles
    if(!wst->tree) {
        free(wst);
        return NULL;
    }
    _init_tree(wst);
    return wst;
}



void wst_free(WeightedSamplingTree* wst)
{
    assert(wst != NULL);
    free(wst->tree);
    wst->tree = NULL;
    free(wst);
}



void wst_change_weight(WeightedSamplingTree* wst, size_t index, double new_weight)
{
    assert(wst != NULL);
    assert(index < wst->size);
    assert(wst != NULL && index < wst->size);
    double delta = new_weight - wst->weights[index];
    wst->weights[index] = new_weight;
    wst->tree[_weights_parent(wst, index)] += delta;
#ifndef NDEBUG
    double weight_lchild = wst->weights[_tree_idx_to_weights_idx(wst, _tree_lchild(_weights_parent(wst, index)))];
    double weight_rchild = wst->weights[_tree_idx_to_weights_idx(wst, _tree_rchild(_weights_parent(wst, index)))];
    assert(wst->tree[_weights_parent(wst, index)] <= weight_lchild + weight_rchild + 0.0001);
    assert(wst->tree[_weights_parent(wst, index)] >= weight_lchild + weight_rchild - 0.0001);
#endif
    for(size_t node = _tree_parent(_weights_parent(wst, index)); node > 0;
        node = _tree_parent(node)) { // can't include root (0) here because of underflow
        wst->tree[node] += delta;
#ifndef NDEBUG
        assert(wst->tree[node] >= -0.000001);
        const double value_error = wst->tree[node] -
                                   (wst->tree[_tree_lchild(node)] + wst->tree[_tree_rchild(node)]);
        assert(value_error < 0.0001 && value_error > -0.0001);
#endif
    }
    wst->tree[0] += delta;
}



// returns a random double x, with +0.0 <= x <= 1.0, uniform distribution
static double _random(void)
{
    return rand() / (double)RAND_MAX; // TODO: use a better rng. RAND_MAX is only guaranteed to be at least 32767 and rand() is also not that fast.
}



// returns random index
// does not remove the selected element
size_t wst_sample_with_replacement(const WeightedSamplingTree* wst)
{
    assert(wst->tree[0] > 0); // the sum of all weights in the tree must not be 0
    // double random = 21654566456126159.3;                       // DEBUG
    double random = _random() * wst->tree[0];
    assert(random >= 0.0);
    assert(random <= wst->tree[0]);
    size_t node = 0;
    const size_t limit = _weights_parent(wst, 0);
    while(node < limit) { // while it is an internal node not on the last level of internal nodes
        assert(node == 0 || random >= 0.0);
        assert(node == 0 || random <= wst->tree[node] + 0.001); // allow for small error because floats
        const size_t lchild = _tree_lchild(node);
        const double sum_left_subtree = wst->tree[lchild];
        if(random <= sum_left_subtree) {
            node = lchild;
        }
        else {
            random -= sum_left_subtree;
            node = _tree_rchild(node);
        }
    }
    const size_t weights_lchild = _tree_idx_to_weights_idx(wst, _tree_lchild(node));
    if(random <= wst->weights[weights_lchild]) {
        assert(weights_lchild < wst->size);
        return weights_lchild;
    }
    else {
        assert(weights_lchild + 1 == _tree_idx_to_weights_idx(wst, _tree_rchild(node)));
        assert(weights_lchild + 1 < wst->size);
        return weights_lchild + 1; // return index of rchild
    }
}



// changes value in weights array
size_t wst_sample_without_replacement(WeightedSamplingTree* wst)
{
    size_t result_index = wst_sample_with_replacement(wst);
    wst_change_weight(wst, result_index, 0.0);
    return result_index;
}
