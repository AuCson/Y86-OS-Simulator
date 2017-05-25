#ifndef BIT_H
#define BIT_H
#define WORD_SIZE 32
typedef unsigned WORD;
typedef char BYTE;

WORD get_bit(WORD w,int low,int high)
{
    if(low == high)
        return 0;
    if(high-low==WORD_SIZE - 1){
        return w;
    }
    w >>= low;
    WORD mask = ~((-1)<<(high-low+1));
    return mask & w;
}

#endif // BIT_H
