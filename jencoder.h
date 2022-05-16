#ifndef JENCODER_H
#define JENCODER_H


#include <inttypes.h>
#include <stdio.h>
#include "cbconf.h"
#include  "lilitypes.h"

class mpeger;
class jpeger;
class jencoder
{
public:
    jencoder(int q, bool bw);
    ~jencoder();
    bool init(const dims_t&);
    int  cam_to_jpg(imglayout_t& img, const std::string&);
    int  cam_to_bw_for_motion(imglayout_t& img);
private:

    jpeger*  _jpgenc;
};

#endif // JENCODER_H
