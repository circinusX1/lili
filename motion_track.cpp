
#include "lilitypes.h"
#include "motion_track.h"

motion_track::motion_track(int motionlow, int motionhi, const dims_t& wh):_mt(wh.x, wh.y)
{
    _motionlow = motionlow;
    if(_motionlow==0)        _motionlow=1;
    _motionhi = motionhi;
    if(_motionhi==0)         _motionhi=8000;
}

int motion_track::movement(const uint8_t* buff, EIMG_FMT fmt)
{
    if(fmt==e422){
        int movedpix =  _mt.det_mov_422((uint8_t*)buff, fmt);
        if(movedpix > _motionlow && movedpix < _motionhi)
        {
             return uint8_t(movedpix);
        }
    }
    return 0;
}

const uint8_t* motion_track::getm(int& w, int& h, int& sz)
{
    _lasttime = time(0);
    w  = _mt.getw();
    h  = _mt.geth();
    sz = w * h;
    return _mt.motionbuf();
}
