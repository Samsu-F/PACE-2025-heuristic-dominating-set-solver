#ifndef _ASSERT_ALLOW_FLOAT_EQUAL
#define _ASSERT_ALLOW_FLOAT_EQUAL

// assert statements in which comparing double or floats with == or != is allowed, without triggering
// the gcc warning -Wfloat-equals




#define assert_allow_float_equal(expr)                                                                   \
    do {                                                                                                 \
        _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wfloat-equal\"") assert(expr); \
        _Pragma("GCC diagnostic pop")                                                                    \
    } while(0)



#endif
