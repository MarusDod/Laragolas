#pragma once

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CAPACITY 10
#define DEFAULT_GROWTH 0.2

#define vector_near_capacity(arr)\
    ((arr)->length >= (uint16_t) (arr)->capacity * 0.7)

#define vector(T)\
    struct {\
        T* data;\
        uint16_t length,capacity;\
    }

#define vector_init(arr)\
    do{\
        (arr)->length=0;\
        (arr)->capacity=DEFAULT_CAPACITY;\
        (arr)->data = calloc(DEFAULT_CAPACITY,sizeof(*(arr)->data));\
    }while(0)

#define vector_increase_capacity(arr)\
    do{\
        ++(arr)->capacity;\
        (arr)->capacity *= 1 + DEFAULT_GROWTH;\
        (arr)->data = realloc((arr)->data,(arr)->capacity * sizeof(*(arr)->data));\
    }while(0)

#define vector_decrease_capacity(arr)\
    do{\
        if((arr)->length < (1-DEFAULT_GROWTH) * (arr)->capacity){\
            (arr)->capacity = (arr)->length * (1 + DEFAULT_GROWTH) + 1;\
            (arr)->data = realloc((arr)->data,(arr)->capacity * sizeof(*(arr)->data));\
        }\
    }while(0)

#define vector_pushback(arr,elt)\
    do{\
        if (vector_near_capacity((arr))){\
            vector_increase_capacity((arr));\
        }\
        (arr)->data[(arr)->length] = elt;\
        ++(arr)->length;\
    }while(0)

#define vector_length(arr)\
    ((arr)->length)

#define vector_pop(arr)\
    ((arr)->data[(arr)->length--])

#define vector_empty(arr)\
    do{\
        free((arr)->data);\
        (arr)->length=0;\
        (arr)->capacity=0;\
    }while(0)

#define vector_foreach(arr,a)\
    for(uint16_t a=0;a < (arr)->length;a++)

#define vector_iter(arr,fn) \
    do{                     \
        vector_foreach((arr),i){\
            fn(vector_get((arr),i)); \
        }\
    }while(0)
#define vector_get(arr,i)\
    (arr)->data[i]

#define vector_set(arr,x)\
    do{\
        (arr)->data[i] = x\
    }while(0)

#define vector_remove_at(arr,i)\
    do{\
        assert((arr)->length > i && i >= 0);\
        --((arr)->length);\
\
        for(int j = i; j < (arr)->length ; j++){\
            ((arr)->data[j]) = (arr)->data[j+1];\
        }\
\
    }while(0)
