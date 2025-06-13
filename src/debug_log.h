#ifndef _DEBUG_LOG_H
#define _DEBUG_LOG_H



// enable logs at compile time using -DDEBUG_LOG
#ifdef DEBUG_LOG
#define debug_log(...)                \
    do {                              \
        fprintf(stderr, __VA_ARGS__); \
    } while(0)
#else
#define debug_log(...) ((void)0)
#endif



#endif
