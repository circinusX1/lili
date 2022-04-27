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
    mmotion(int w, int h);
    ~mmotion();
    int det_mov_422(uint8_t* p, EIMG_FMT fmt);
    int  getw()const{return _mw;}
    int  geth()const{return _mh;}
    uint8_t*  motionbuf()const{return _motionbufs[2];}
    int darkav()const{return _dark;}

private:
    int       _w;
    int       _h;
    int       _mw;
    int       _mh;
    uint8_t*  _motionbufs[3];
    int       _motionindex;
    uint32_t  _motionsz;
    mutexx     _m;
    int       _moves;
    int       _dark;
    int       _noisediv=4;
    int       _mmeter;
    rect_t    _inrect;
    rect_t    _outrect;
};



#endif // V4LDEVICE_H
