#include "application.h"

#ifndef SIMPLERINGBUFFER_H
#define	SIMPLERINGBUFFER_H


class SimpleRingBuffer {
    protected:
        uint8_t* _data;
        unsigned int data_size;
        unsigned int cur_idx;
        unsigned int cur_len;

    public:
        SimpleRingBuffer();

        void init(unsigned int size);

        bool put(uint8_t value);
        uint8_t get();

        unsigned int getSize();
        unsigned int getCapacity();

        void clear();
        void destroy();
};

#endif
