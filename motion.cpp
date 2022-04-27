#include <cstdio>
#include <jpeglib.h>
#include <jerror.h>
#define cimg_plugin "jpeg_buffer.h"
#include "CImg.h"
using namespace cimg_library;
#include <assert.h>
#include <string.h>
#include "motion.h"
#include "cbconf.h"


mmotion::mmotion(const dims_t& wh, const Cbdler::Node& n):_w(wh.x),_h(wh.x)
{
	_noisediv = n["noise_div"].to_int();
	_motionsc = n["motion_scale"].to_int();
	_inrect = n["in_rect"].to_rect();
	_outrect = n["out_rect"].to_rect();
	_mdiff    = n["motion_diff"].to_int() * 2.55;

	if(_motionsc<2) _motionsc=2;
	else if(_motionsc>16) _motionsc=16;

	if(_noisediv<4)_noisediv=4;
	_mw = wh.x/_motionsc;
	_mh = wh.y/_motionsc;

	size_t msz = (_mw) * (_mh);

	if(_inrect.x < 0)   _inrect.x=0;
	if(_inrect.y < 0)   _inrect.y=0;
	if(_inrect.X >= _w)	_inrect.X=_w;
	if(_inrect.Y >= _h) _inrect.Y=_h;
	if(_outrect.x < 0)    _outrect.x=0;
	if(_outrect.y < 0)    _outrect.y=0;
	if(_outrect.X >= _w)  _outrect.X=_w;
	if(_outrect.Y >= _h)  _outrect.Y=_h;

	_outrect.x/=_motionsc;
	_outrect.y/=_motionsc;
	_outrect.X/=_motionsc;
	_outrect.Y/=_motionsc;
	_inrect.x/=_motionsc;
	_inrect.y/=_motionsc;
	_inrect.X/=_motionsc;
	_inrect.Y/=_motionsc;

	_motionbufs[0] = new uint8_t[msz];
	_motionbufs[1] = new uint8_t[msz];
	_motionbufs[2] = new uint8_t[msz];
	memset(_motionbufs[0],0,msz);
	memset(_motionbufs[1],0,msz);
	memset(_motionbufs[2],0,msz);
	_mobuf_idx = 0;
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

int mmotion::_det_mov_422(const uint8_t* fmt420)
{
    const uint8_t* base_py = fmt420;
    uint8_t* pSeen = _motionbufs[2];
    uint8_t* prowprev = _motionbufs[_mobuf_idx ? 0 : 1];
    uint8_t* prowcur = _motionbufs[_mobuf_idx ? 1 : 0];
    int               dx = _w / _mw;
    int               dy = _h / _mh;
    int               pixels = 0;

    uint8_t           Y,YP ;

    if(_mdiff<1){  _mdiff=4; }
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

            if(diff < _mdiff)
            {
                diff=Y;                         // black no move
            }
            else if(diff>_mdiff)
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
    // show spin percentage on left as bar
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
    _mobuf_idx = !_mobuf_idx;
    //acale moves to (uint8-10)
    return _moves;
}

int mmotion::det_mov(const uint8_t* p, size_t len, EIMG_FMT fmt)
{
    _moves = 0;
    if(p && len)
    {
         _moves = 0;
        if(fmt==e422){
            return _det_mov_422(p);
        }
        else if(fmt==eFJPG)
        {
        /*

             TRACE() <<"IMAGE " << len << "\r\n";
            CImg<unsigned char> img;
            img.load_jpeg_buffer(p, len);

           const CImg<unsigned long>& ni = img.get_histogram(255);
            unsigned long sz = ni.size();
            const uint8_t* pbhisto = nullptr;

            uint8_t* pSeen    = _motionbufs[2];
            uint8_t* prowprev = _motionbufs[_mobuf_idx ? 0 : 1];
            uint8_t* prowcur  = _motionbufs[_mobuf_idx ? 1 : 0];
            int      hstolen = ni.get_raw(&pbhisto);
            if(hstolen < _motionsz){
                ::memcpy(prowcur, pbhisto, hstolen);
                ::memcpy(pSeen, pbhisto, hstolen);
                _moves = memcmp(prowprev, prowcur, hstolen);
            }
            _mobuf_idx = !_mobuf_idx;
*/
        }
    }
    return _moves;
}




