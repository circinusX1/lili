#ifndef MOTION_TRACK_H
#define MOTION_TRACK_H

#include "motion.h"
#include "cbconf.h"


class motion_track
{
public:
    motion_track(int motionlow, int motionhi, const dims_t& wh);

    int movement(const uint8_t* buff, EIMG_FMT fmt);
    int darkaverage()const{return _mt.darkav();}
    const uint8_t* getm(int& w, int& h, int& sz);

private:
    mmotion     _mt;
    int         _motionlow = 0;
    int         _motionhi = 0;
    time_t      _lasttime = 0;
};

#endif // MOTION_TRACK_H
