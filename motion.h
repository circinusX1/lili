/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/

    Author:  Marius O. Chincisan
    First Release: September 16 - 29 2016
*/
#ifndef MOTION_H
#define MOTION_H

#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include "motion.h"
#include "cbconf.h"
#include "lilitypes.h"
#include "os.h"


class mmotion
{
public:
    mmotion(const dims_t& wh,  const Cbdler::Node& n);
    ~mmotion();

    int  getw()const{return _mw;}
    int  geth()const{return _mh;}
    uint8_t*  motionbuf()const{return _motionbufs[2];}
    int darkav()const{return _dark;}
    int det_mov(const imglayout_t& imgl);

private:
    int _det_mov_422(const imglayout_t& imgl);
    void _meter_show(uint8_t* pSeen);
    void _calc_rects(int w, int h);

private:
    int       _w;
    int       _h;
    int       _mw;
    int       _mh;
    int       _motionsc = 4;
    uint8_t*  _motionbufs[3] = {nullptr, nullptr,nullptr};
    int       _mobuf_idx;
    int       _motionsz;
    mutexx    _m;
    int       _moves;
    int       _dark;
    int       _noisediv=4;
    int       _mmeter;
    int       _mdiff = 4;
    rect_t    _inrect;
    rect_t    _outrect;
    bool      _recalc = false;
};



#endif // V4LDEVICE_H
