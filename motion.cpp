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
	_inrect   = n["in_rect"].to_rect();
	_outrect  = n["out_rect"].to_rect();
	_mdiff    = n["motion_diff"].to_int() * 2.55;

	if(_motionsc<1)
		_motionsc=1;
	else if(_motionsc>16)
		_motionsc=16;

	if(_noisediv<4)_noisediv=4;
	_calc_rects(wh.x,wh.y);
}

mmotion::~mmotion()
{
    delete []_motionbufs[0];
    delete []_motionbufs[1];
    delete []_motionbufs[2];
}


void mmotion::_motion(uint8_t pix,
                     const uint8_t* base_py, uint8_t* pSeen, uint8_t* prowprev, uint8_t* prowcur,
                      int x, int y, int dx, int dy, int & pixels)
{
    uint8_t Y  = base_py ? *(base_py + ((y*dy)  * _w) + (x*dx)): pix; /// curent pixel

    _dark += uint32_t(Y);		   //  noise and dark
    Y /= _noisediv;
    Y *= _noisediv;

    *(prowcur + (y * _mw) + x) = Y;       // build new video buffer
    uint8_t YP = *(prowprev+(y  * _mw) + (x));   // old buffer pixel

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

int mmotion::_det_mov_422(const imglayout_t& imgl)
{

    uint8_t* pSeen = _motionbufs[2];
    uint8_t* prowprev = _motionbufs[_mobuf_idx ? 0 : 1];
    uint8_t* prowcur = _motionbufs[_mobuf_idx ? 1 : 0];
    const uint8_t* fmt420 = imgl._camp;
    const uint8_t* base_py = fmt420;
    int dx = imgl._dims.x / _mw; if(dx==0)dx=1;
    int dy = imgl._dims.y / _mh; if(dy==0)dy=1;
    int               pixels = 0;

    if(_mdiff<1){  _mdiff=4; }
    _dark  = 0;
    _moves = 0;
    for (int y=1; y <_mh-dy; y++)             //height
    {
        for (int x = 1; x < _mw-dx; x++)       //width
        {
            if( _inrect.x!= _inrect.X){
                if(x<_inrect.x)continue;
                if(x>_inrect.X)continue;
                if(y<_inrect.y)continue;
                if(y>_inrect.Y)continue;
            }
            if(_outrect.x != _outrect.X){
                if(x > _outrect.x && x < _outrect.X &&
                        y > _outrect.y && y < _outrect.Y)
                    continue;
            }
            _motion(0, base_py, pSeen, prowprev, prowcur, x-1, y-1, dx, dy, pixels);

        }

    }

    //[x,y,X,Y] draw a rect

    _meter_show(pSeen);
    _dark /= pixels;
    _mobuf_idx = !_mobuf_idx;
    //acale moves to (uint8-10)
    return _moves;
}

//decrease mh in 3 seconds
void mmotion::_meter_show(uint8_t* pSeen)
{
    static time_t  last = gtc();

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
        for (int y= _outrect.y+2; y <_outrect.Y-1; y++)
        {
            *(pSeen + (y * _mw) + _outrect.x) = (uint8_t)128;
            *(pSeen + (y * _mw) + _outrect.X) = (uint8_t)128;
        }

        for (int x = _outrect.x+2; x < _outrect.X-2; x++)
        {
            *(pSeen + (_outrect.y * _mw) + x) = (uint8_t)128;
            *(pSeen + (_outrect.Y * _mw) + x) = (uint8_t)128;
        }
    }
    // show spin percentage on left as bar
    int percentage = std::min(100,_moves);
    if(percentage > _mmeter)
        _mmeter = percentage;
    else if(_mmeter>0){
        time_t now = gtc();
        _mmeter -= ((now-last)/24 + 1);
        if(_mmeter<0)_mmeter=0;
        last = now;
    }
    if(_mmeter>0)
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
}

// to do image is smaller from digoo cam TODO
int mmotion::det_mov(const imglayout_t& imgl)
{
    const uint8_t* p = imgl._camp;
    size_t len      = imgl._caml;
    EIMG_FMT fmt    = imgl._camf;

    _moves = 0;
    if(p && len)
    {
        _calc_rects(imgl._dims.x, imgl._dims.y);

        if(fmt==e422){
            return _det_mov_422(imgl);
        }
        else if(fmt==eFJPG && ::is_jpeg(p,len))
        {
            int components = 1;
            CImg<unsigned char> img;
            img.load_jpeg_buffer(p, len, components);// .get_RGBtoxyY();

            uint8_t* pSeen = _motionbufs[2];
            uint8_t* prowprev = _motionbufs[_mobuf_idx ? 0 : 1];
            uint8_t* prowcur = _motionbufs[_mobuf_idx ? 1 : 0];
            int pixels = 0;


            int dx = img.width() / _mw; if(dx==0)dx=1;
            int dy = img.height() / _mh; if(dy==0)dy=1;
            int neww = _mw * components;
            //dx*=3;

            if(_mdiff<1){  _mdiff=4; }
            for (int y = 1; y <_mh-dy; y++)             //height
            {
                for (int x = 1; x < neww-x; x++)       //width
                {
                    if( _inrect.x!= _inrect.X){
                        if(x<_inrect.x)continue;
                        if(x>_inrect.X)continue;
                        if(y<_inrect.y)continue;
                        if(y>_inrect.Y)continue;
                    }
                    if(_outrect.x != _outrect.X){
                        if(x > _outrect.x && x < _outrect.X &&
                                y > _outrect.y && y < _outrect.Y)
                            continue;
                    }
                    _motion(img(x*dx, y*dy),
                            nullptr, pSeen, prowprev,
                            prowcur, x-1, y-1, dx, dy, pixels);
                }
            }

            _meter_show(pSeen);
            _dark /= pixels;
            _mobuf_idx = !_mobuf_idx;
        }
        else
        {
            ::memset(_motionbufs[2], 0x80, _motionsz);
        }
    }
    return _moves;
}


void mmotion::_calc_rects(int w, int h)
{
	if(_w == w && _h == h)
		return;

	int neww = w/_motionsc;
	int newh = h/_motionsc;
	_w = w;
	_h = h;

	if(neww != _mw || newh != _mh)
	{
		_mw = w/_motionsc;
		_mh = h/_motionsc;
		if(_mw==0||_mh==0){
			return;
		}
		size_t msz = (_mw) * (_mh);

		if(_inrect.x < 0)    _inrect.x=0;
		if(_inrect.y < 0)    _inrect.y=0;
		if(_inrect.X >= _w)	 _inrect.X=_w;
		if(_inrect.Y >= _h)  _inrect.Y=_h;
		if(_outrect.x < 0)   _outrect.x=0;
		if(_outrect.y < 0)   _outrect.y=0;
		if(_outrect.X >= _w) _outrect.X=_w;
		if(_outrect.Y >= _h) _outrect.Y=_h;

		_outrect.x/=_motionsc;
		_outrect.y/=_motionsc;
		_outrect.X/=_motionsc;
		_outrect.Y/=_motionsc;
		_inrect.x/=_motionsc;
		_inrect.y/=_motionsc;
		_inrect.X/=_motionsc;
		_inrect.Y/=_motionsc;

		delete[] _motionbufs[0];
		delete[] _motionbufs[1];
		delete[] _motionbufs[2];

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
}
