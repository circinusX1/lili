
#include <assert.h>
#include <string.h>
#include "motion.h"
#include "cbconf.h"


mmotion::mmotion(int w, int h, int nr):_w(w),_h(h),_noisediv(nr)
{
	_mw = _w/4;
	_mh = _h/4;

	int motionsc = CFG["move"]["motion_scale"].to_int();
	size_t msz = (_mw) * (_mh);
	_inrect = CFG["move"]["in_rect"].to_rect();
	_outrect = CFG["move"]["out_rect"].to_rect();

	if(_inrect.x < 0)   _inrect.x=0;
	if(_inrect.y < 0)   _inrect.y=0;
	if(_inrect.X >= _w)	_inrect.X=_w;
	if(_inrect.Y >= _h) _inrect.Y=_h;
	if(_outrect.x < 0)    _outrect.x=0;
	if(_outrect.y < 0)    _outrect.y=0;
	if(_outrect.X >= _w)  _outrect.X=_w;
	if(_outrect.Y >= _h)  _outrect.Y=_h;
	if(motionsc<2) motionsc=2;
	else if(motionsc>16) motionsc=16;

	if(_noisediv<4)_noisediv=4;

	_outrect.x/=motionsc;
	_outrect.y/=motionsc;
	_outrect.X/=motionsc;
	_outrect.Y/=motionsc;
	_inrect.x/=motionsc;
	_inrect.y/=motionsc;
	_inrect.X/=motionsc;
	_inrect.Y/=motionsc;

	_motionbufs[0] = new uint8_t[msz];
	_motionbufs[1] = new uint8_t[msz];
	_motionbufs[2] = new uint8_t[msz];
	memset(_motionbufs[0],0,msz);
	memset(_motionbufs[1],0,msz);
	memset(_motionbufs[2],0,msz);
	_motionindex = 0;
	_motionsz = msz;
	_moves=0;
	_mmeter = 0;
}

mmotion::~mmotion()
{
    delete []_motionbufs[0];
    delete []_motionbufs[1];
    delete []_motionbufs[2];
}

int mmotion::has_moved(uint8_t* fmt420)
{
    register uint8_t *base_py = fmt420;
    register uint8_t*          pSeen = _motionbufs[2];
    register uint8_t*          prowprev = _motionbufs[_motionindex ? 0 : 1];
    register uint8_t*          prowcur = _motionbufs[_motionindex ? 1 : 0];
    int               dx = _w / _mw;
    int               dy = _h / _mh;
    int               pixels = 0;
    int               mdiff  = CFG["move"]["motion_diff"].to_int() * 2.55;
    uint8_t           Y,YP ;

    if(mdiff<1){  mdiff=4; }
    _dark  = 0;
    _moves = 0;
    for (int y= 0; y <_mh-dy; y++)             //height
    {
        for (int x = 0; x < _mw-dx; x++)       //width
        {
            if(x<_inrect.x)continue;
            if(x>_inrect.X)continue;
            if(y<_inrect.y)continue;
            if(y>_inrect.Y)continue;

            if(x > _outrect.x && x < _outrect.X &&
                    y > _outrect.y && y < _outrect.Y)
                continue;

            Y  = *(base_py + ((y*dy)  * _w) + (x*dx)); /// curent pixel

            _dark += uint32_t(Y);		   //  noise and dark
            Y /= _noisediv; Y *= _noisediv;

            *(prowcur + (y * _mw) + x) = Y;       // build new video buffer
            YP = *(prowprev+(y  * _mw) + (x));   // old buffer pixel

            int diff = abs(Y - YP);

            if(diff < mdiff)
            {
                diff=Y;                         // black no move
            }
            else if(diff>mdiff)
            {
                diff=255; //move
                ++_moves;
            }
            *(pSeen + (y * _mw)+x) = (uint8_t)diff;
            ++pixels;
        }

    }

    //[x,y,X,Y] draw a rect
    if(_inrect.x != _inrect.X)
    {
        for (int y= _inrect.y+1; y <_inrect.Y-1; y++)
        {
            *(pSeen + (y * _mw) + _inrect.x) = (uint8_t)192;
            *(pSeen + (y * _mw) + _inrect.X) = (uint8_t)192;
        }

        for (int x = _inrect.x+1; x < _inrect.X-1; x++)
        {
            *(pSeen + (_inrect.y * _mw) + x) = (uint8_t)192;
            *(pSeen + (_inrect.Y * _mw) + x) = (uint8_t)192;
        }
    }

    if(_outrect.x != _outrect.X)
    {
        for (int y= _outrect.y+1; y <_outrect.Y-1; y++)
        {
            *(pSeen + (y * _mw) + _outrect.x) = (uint8_t)128;
            *(pSeen + (y * _mw) + _outrect.X) = (uint8_t)128;
        }

        for (int x = _outrect.x+1; x < _outrect.X-1; x++)
        {
            *(pSeen + (_outrect.y * _mw) + x) = (uint8_t)128;
            *(pSeen + (_outrect.Y * _mw) + x) = (uint8_t)128;
        }
    }
    // show movement percentage on left as bar
    int percentage = std::min(100,_moves);
    if(percentage > _mmeter)
        _mmeter = percentage;
    else if(_mmeter)
        --_mmeter;
    if(_mmeter)
    {
        int y =_mh;
        int x = 0;
        while(y)
        {
            if(_mmeter < y)
                *(pSeen + ((_mh-y) * _mw)+x) = (uint8_t)0;
            else
                *(pSeen + ((_mh-y) * _mw)+x) = (uint8_t)255;
            --y;
        }
    }

    _dark /= pixels;
    _motionindex = !_motionindex;
    return _moves;
}

