#ifndef ENCODER_H
#define ENCODER_H


#include <inttypes.h>
#include <stdio.h>
#include "cbconf.h"

class encoder
{
public:
    encoder(){};
    virtual ~encoder(){};
    virtual bool init(const dims_t&)=0;
    virtual int fmt42_to_jpg(const uint8_t* fmt420, int w,int h, const uint8_t** ppng)=0;
    virtual int fmt42_to_bw(const uint8_t* uint8buf,  int w, int h, const uint8_t** pjpeg)=0;
private:
};

#endif // ENCODER_H
