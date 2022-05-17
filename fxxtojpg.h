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
#ifndef FXXTOJPG_H
#define FXXTOJPG_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <string>
#include <time.h>
#include <jpeglib.h>
#include <jerror.h>
#include "cbconf.h"

class jpeger
{
public:
    jpeger(int q, bool bw);
    virtual ~jpeger();
    bool init(const dims_t&);
    int cam_to_jpg(imglayout_t& img, const std::string&);
    int cam_to_bw_for_motion(imglayout_t& img);

private:
	int _put_jpeg_yuv420p_memory(const uint8_t *pyuv420,
								int width, int height,
								int jpg_quality, struct tm *tm);
	void _jpeg_mem_dest(j_compress_ptr cinfo);

public:
    uint8_t*        _image=nullptr;
    int             _jpgq = 0;
    uint32_t        _imgsize = 0;
    unsigned long   _memsz = 0;
    bool            _bandw = false;
    struct jpeg_compress_struct _cinfo;
    struct          jpeg_error_mgr _jerr;
};

#endif // FXXTOJPG_H
