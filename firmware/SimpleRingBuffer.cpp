#include "application.h"
#include "SimpleRingBuffer.h"

SimpleRingBuffer::SimpleRingBuffer() {

}

void SimpleRingBuffer::init(unsigned int size) {
    cur_idx = 0;    //pos
    cur_len = 0;    //len

    _data = (uint8_t*)malloc(size * sizeof(uint8_t));
    data_size = size; //cap
}

bool SimpleRingBuffer::put(uint8_t value) {
    if (cur_len < data_size) {
        // get our index, wrap on buffer size
        _data[(cur_idx + cur_len)%data_size] = value;
        cur_len++;
        return true;
    }
    return false;
}

uint8_t SimpleRingBuffer::get() {
    // lets return 0 if we don't have anything
    uint8_t val = 0;


    if (cur_len > 0) {

        // grab the value from the current index
        val = _data[cur_idx];

        // move to next, wrap as necessary, shrink length
        cur_idx = (cur_idx + 1) % data_size;
        cur_len--;
    }
    return val;
}

unsigned int SimpleRingBuffer::getSize() {
    return cur_len;
}

unsigned int SimpleRingBuffer::getCapacity() {
    return data_size;
}

void SimpleRingBuffer::clear() {
    cur_idx = 0;
    cur_len = 0;
}

void SimpleRingBuffer::destroy() {
    free(_data);
    _data = NULL;
}
