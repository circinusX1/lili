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
#ifndef V4LDEVICE_H
#define V4LDEVICE_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>
#include <linux/videodev2.h>
#include <jpeglib.h>
#include <libv4l2.h>
#include <signal.h>
#include <stdint.h>
#include <inttypes.h>
#include <string>
#include <vector>
#include "motion.h"
#include "os.h"

#define MAX_BUFFERS 4

struct videobuffer {
    void *  start;
    size_t  length;
    int     mmap;
};


class v4ldevice
{
public:

    v4ldevice(const char* device, int x, int y, int fps);
    virtual ~v4ldevice();

    bool open();
    void close();
    const uint8_t* read(int& w, int& h, int& sz, bool& fatal); // ret 0 fatal, 1 aquired, -1 continue
    void set_fps(int fps);

private:
    int _ioctl(int request, void* argp);

private:
    std::string  _sdevice;
    int       _device;
    int       _xy[2];
    int       _fps;
    int       _curbuffer;
    timeval   _curts;
    uint32_t  _buffsize;
    struct    videobuffer  _buffers[MAX_BUFFERS];
    bool      _fatal;
};

#endif // V4LDEVICE_H
