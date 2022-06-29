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

///////////////////////////////////////////////////////////////////////////////////////////////////
mmotion::mmotion(const dims_t& wh, const Cbdler::Node& n):_w(wh.x),_h(wh.x)
{
	_noisediv = n["noise_div"].to_int();
	_imgscale = n["img_scale"].to_int();
	_inrect   = n["in_rect"].to_rect();
	_outrect  = n["out_rect"].to_rect();
	_pixnoise = n["pix_noise"].to_int() * 2.55;
	if(_imgscale<1)				_imgscale=1;
	else if(_imgscale>16)		_imgscale=16;
	if(_noisediv<4)_noisediv=4;
	else if(_noisediv>32)_noisediv=32;
	_calc_rects(wh.x,wh.y);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
mmotion::~mmotion()
{
    delete []_motionbufs[0];
    delete []_motionbufs[1];
    delete []_motionbufs[2];
}

///////////////////////////////////////////////////////////////////////////////////////////////////
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
    if(diff < _pixnoise)
    {
        diff = Y/2;                         // black no move
    }
    else if(diff>_pixnoise)
    {
        diff=255; //move
        ++_moves;
    }
    *(pSeen + (y * _mw)+x) = (uint8_t)diff;
    ++pixels;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
int mmotion::_det_mov_422(const imglayout_t& imgl, const dims_t& lohi)
{
    uint8_t* pSeen = _motionbufs[2];
    uint8_t* prowprev = _motionbufs[_mobuf_idx];
    uint8_t* prowcur = _motionbufs[!_mobuf_idx];
    const uint8_t* fmt420 = imgl._camp;
    const uint8_t* base_py = fmt420;
    int dx = imgl._dims.x / _mw; if(dx==0)dx=1;
    int dy = imgl._dims.y / _mh; if(dy==0)dy=1;
    int               pixels = 0;

    if(_pixnoise<1){  _pixnoise=4; }
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
    _meter_show(pSeen, lohi);
    _dark /= pixels;
    _mobuf_idx = !_mobuf_idx;
    return _moves;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void mmotion::_meter_show(uint8_t* pSeen, const dims_t& lohi)
{
    //drow seen rects
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

    if(_moves < lohi.x)_moves=0;
    else if(_moves < lohi.x)_moves=0;

    // show percentage on left as bar
    if(_moves > _maxmoves && _moves < lohi.y)
    {
        _maxmoves = _moves;
    }
    //scale to _maxmoves for _mh
    // maxmoves -> _mh
    // curmove  ->  x
    _mmeter = (_moves*_mh)/_maxmoves;

    if(_mmeter>0)
    {
        int y =_mh;
        while(y)
        {
            if(_mmeter < y)
                *(pSeen + ((_mh-y) * _mw)) = (uint8_t)0;
            else{
                *(pSeen + ((_mh-y) * _mw)+1) = (uint8_t)255;
            }
            --y;
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// to do image is smaller from digoo cam TODO
int mmotion::det_mov(const imglayout_t& imgl, const dims_t& lohi)
{
    const uint8_t* p = imgl._camp;
    size_t len      = imgl._caml;
    EIMG_FMT fmt    = imgl._camf;

    _moves = 0;
    if(p && len)
    {
        _calc_rects(imgl._dims.x, imgl._dims.y);

        if(fmt==e422){
            return _det_mov_422(imgl, lohi);
        }
        else if(fmt==eFJPG && ::is_jpeg(p,len))
        {
            int components = 1;
            CImg<unsigned char> img;
            img.load_jpeg_buffer(p, len, components);// .get_RGBtoxyY();

            uint8_t* pSeen = _motionbufs[2];
            uint8_t* prowprev = _motionbufs[_mobuf_idx];
            uint8_t* prowcur = _motionbufs[!_mobuf_idx];
            int pixels = 0;


            int dx = img.width() / _mw; if(dx==0)dx=1;
            int dy = img.height() / _mh; if(dy==0)dy=1;
            int neww = _mw * components;
            //dx*=3;

            if(_pixnoise<1){  _pixnoise=4; }
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

            _meter_show(pSeen, lohi);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
void mmotion::get(int& pixnoise, int& pixdiv, int& imgscale)
{
    pixnoise = _pixnoise;
    pixdiv = _noisediv;
    imgscale = _imgscale;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void mmotion::set(int pixnoise, int pixdiv, int imgscale)
{
    bool apply = false;

    if(pixnoise>0)   {_pixnoise = pixnoise; apply=true;};
    if(pixdiv>0)     {_noisediv = pixdiv; apply=true;};
    if(_imgscale>0)  {_imgscale = imgscale; apply=true;};
    _calc_rects(_w, _h, apply);
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void mmotion::_calc_rects(int w, int h, bool force)
{
	if(_w == w && _h == h && force==false)
		return;

	int neww = w/_imgscale;
	int newh = h/_imgscale;
	_w = w;
	_h = h;

	if(neww != _mw || newh != _mh)
	{
		_mw = w/_imgscale;
		_mh = h/_imgscale;
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

		_outrect.x/=_imgscale;
		_outrect.y/=_imgscale;
		_outrect.X/=_imgscale;
		_outrect.Y/=_imgscale;
		_inrect.x/=_imgscale;
		_inrect.y/=_imgscale;
		_inrect.X/=_imgscale;
		_inrect.Y/=_imgscale;

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
		_maxmoves = _mh;
	}
}
