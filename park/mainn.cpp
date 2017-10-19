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


#define LIVEIMAGE_VERSION "1.0.0"
//#define cimg_display 0
//#define cimg_use_jpeg

#include <stdint.h>
#include <unistd.h>
#define  __USE_FILE_OFFSET64
#include <stdlib.h>
#include <sys/statvfs.h>
#include <iostream>
#include <string>
#include "v4ldevice.h"
#include "v4libavdev.h"
#include "sockserver.h"
#include "jpeger.h"
#include "jpeger.h"
#include "liconfig.h"
#include "webcast.h"

/*
sudo apt-get install libpng-dev libv4l-dev libjpeg-dev
*/

using namespace std;
bool __alive = true;
bool _sig_proc_capture=false;

static struct
{
    uint32_t sz;
    int x;
    int y;

}  _rez[]=
{
{  403270  ,1024, 768},
{  504435  ,1152, 864},
{  614806  ,1280, 960},
{  865866  ,1400, 1050},
{  1082915 ,1600, 1200},
{  1501869 ,1920, 1440},
{  1684088 ,2048, 1536},
{  2519088 ,2592, 1944},
{  174204  ,640,  480},
{  237247  ,768,  576},
{  256298  ,800,  600},
};



void ControlC (int i)
{
    (void)i;
    __alive = false;
    printf("Exiting...\n");
}


void ControlP (int i)
{
    (void)i;
}

void Capture (int i)
{
    (void)i;
    _sig_proc_capture=true;
}

uint32_t _imagesz(int x, int y)
{
    for(size_t k=0; k<sizeof(_rez)/sizeof(_rez[0]); k++)
    {
        if(_rez[k].x==x && _rez[k].y==y)
        {
            return _rez[k].sz;
        }
    }
    return 1082915;
}


static void capture(outfilefmt* ffmt, sockserver* ps, v4ldevice& dev, std::string pathname, int firstimage, int maxfiles);
static void calc_room(const std::string& pathname, int& curentfile, uint64_t& maxfiles);
static void av_frm_callback(camonlibav*);

struct{
    uint8_t* pimage;
    size_t   isize;
}   _enc_image;


int main(int nargs, char* vargs[])
{
    signal(SIGINT,  ControlC);
    signal(SIGABRT, ControlC);
    signal(SIGKILL, ControlC);
    signal(SIGUSR2, Capture);
    signal(SIGTRAP, SIG_IGN);
    signal(SIGPIPE, SIG_IGN);

    (void)nargs;
    (void)vargs;
    LiConfig    conf("liveimage.conf");

    if(conf._glb.method=="v4l")
    {

        v4ldevice   dev(conf._glb.device.c_str(),
                        conf._glb.w, conf._glb.h,
                        conf._glb.fps,
                        conf._glb.imotion[0], conf._glb.imotion[1],
                conf._glb.motionnoise);
        if(dev.open())
        {
            std::cout << conf._glb.device << " opened\n";
            sockserver* ps = 0;
            if(conf._glb.port)
            {
                ps = new sockserver(conf._glb.port, "http");
                if(ps && ps->listen()==false)
                {
                    delete ps;
                    return 0;
                }
            }

            outfilefmt*     ffmt = 0;
            ffmt = new jpeger(conf._glb.quality);

            uint64_t maxfiles=0;
            uint32_t maxfiles2=0;
            int firstimage=0;
            if(conf._glb.maxfiles==0)
            {
                if(!conf._glb.pathname.empty())
                    calc_room(conf._glb.pathname, firstimage, maxfiles);
                maxfiles2=(uint32_t)maxfiles;
            }
            else
            {
                maxfiles2 = conf._glb.maxfiles;
            }
            std::cout << "rotating images at:" << maxfiles2 << "\n";
            capture(ffmt, ps, dev, conf._glb.pathname, firstimage, maxfiles2);

            delete ps;
            dev.close();
            delete ffmt;
        }
    }
    else
    {
        camonlibav libav(conf._glb.device.c_str(), av_frm_callback);
        if(libav._ok)
        {
            std::cout << conf._glb.device << " opened\n";
            sockserver* ps = 0;
            if(conf._glb.port)
            {
                ps = new sockserver(conf._glb.port+1, "http");
                if(ps && ps->listen()==false)
                {
                    delete ps;
                    return 0;
                }
            }
            while(__alive && 0 == ::usleep(15000))
            {
                libav.get_frame();
                if(ps)
                    ps->spin();

                if(ps && ps->has_clients())
                {
                    int wants = ps->anyone_needs();
                    ps->stream_on(_enc_image.pimage, _enc_image.isize,"jpeg", wants);
                }
            }
        }
    }
}

static void av_frm_callback(camonlibav* plib)
{
    AVPacket pkt;
    int      got_output = 0;

    av_init_packet(&pkt);
    pkt.data = NULL;    // packet data will be allocated by the encoder
    pkt.size = 0;
    int ret = avcodec_encode_video2(plib->_ctx,
                                    &pkt, plib->pRawFrame, &got_output);
    if (ret>=0 && got_output){
        _enc_image.isize = pkt.size;
        _enc_image.pimage = pkt.data;
    }
}


void calc_room(const std::string& pathname, int& firstimage, uint64_t& maximages)
{
    struct statvfs64 fiData;

    if((statvfs64(pathname.c_str(), &fiData)) == 0 )
    {
        uint64_t bytesfree = (uint64_t)(fiData.f_bfree * fiData.f_bsize);
        uint64_t imgsz = _imagesz(GCFG->_glb.w, GCFG->_glb.h);
        maximages = (uint64_t)(bytesfree/imgsz)/4;
        std::cout << "disk free:" << bytesfree << "\n";

    }
    else
    {
        maximages = 2;
    }

    FILE* pff = ::fopen("./.lastimage","rb");
    if(pff)
    {
        char index[8];
        ::fgets(index, 7, pff);
        firstimage=::atoi(index);
        ::fclose(pff);
    }
    else
        firstimage = 0;
    std::cout << "Current image:" << firstimage << ", Roll up at:" << maximages << "\n";
}


void capture(outfilefmt* ffmt, sockserver* ps, v4ldevice& dev,
             std::string pathname, int firstimage, int maxfiles)
{
    uint32_t        jpgsz = 0;
    uint8_t*        pjpg = 0;
    const uint8_t*  pb422;
    int             robinserve = 0x1;
    time_t          lapsetick =  gtc();
    time_t          tickmove =  gtc();
    int             movepix = 0;
    bool            savemove = false;
    bool            savelapse = false;
    int             movementintertia = 0;
    size_t          sz = 0;
    int             iw = GCFG->_glb.w;
    int             ih = GCFG->_glb.h;
    bool            fatal=false;
    WebCast          cast;


    if(!GCFG->_glb.webcast.empty())
    {
        cast.start_thread();
    }

    while(__alive && 0 == ::usleep(15000))
    {
        if(ps)
            ps->spin();
        pb422 = dev.read(iw, ih, sz,fatal);
        if(pb422 == 0){
            if(fatal){
                std::cout << "fatal error io. exiting \n";
                __alive=false; //let the service script handle it
            }
            ::sleep(1);
            continue;
        }
        jpgsz = ffmt->convert420(pb422, iw, ih, GCFG->_glb.quality, &pjpg);
        if(ps && ps->has_clients())
        {
            int wants = ps->anyone_needs();
            if(wants == WANTS_HTML)
            {
                ps->stream_on(0, 0, "", WANTS_HTML);
            }
            else if(wants & WANTS_LIVE_IMAGE && robinserve  ==  WANTS_LIVE_IMAGE)
            {
                ps->stream_on(pjpg, jpgsz, GCFG->_glb.format=="jpg" ? "jpeg" : "png", WANTS_LIVE_IMAGE);
            }
            else if(wants & WANTS_MOTION && robinserve  ==  WANTS_MOTION)
            {
                uint8_t*        pjpg1 = 0;
                int             w,h;
                const uint8_t*  mot = dev.getm(w, h, sz);
                size_t          jpgsz1 = ffmt->convertBW(mot, w, h, 80, &pjpg1);
                ps->stream_on(pjpg1, jpgsz1, GCFG->_glb.format=="jpg" ? "jpeg" : "png", WANTS_MOTION);
            }
            else if(wants & WANTS_VIDEO_TODO && robinserve  ==  WANTS_VIDEO_TODO)
            {
                ps->stream_on(pjpg, jpgsz, 0, WANTS_VIDEO_TODO);
            }
            robinserve <<= 0x1;
            if(robinserve==WANTS_MAX){
                robinserve = 0x1;
            }
        }
        if(!cast.is_stopped())
        {
            cast.stream_frame(pjpg, jpgsz);
        }


        uint32_t now =  gtc();

        //
        // MOTION
        //
        if(GCFG->_glb.imotion[0]>0)
        {
            if(now - tickmove > GCFG->_glb.motionsnap)
            {
                movepix = dev.movement();
                if( movepix >= GCFG->_glb.imotion[0] && movepix <= GCFG->_glb.imotion[1])
                {
                    std::cout << "move pix=" << movepix << "\n";
                    savemove = true;
                    movementintertia = GCFG->_glb.motiontrail;
                }
                else
                {
                    savemove = false;
                }
                if(movementintertia > 0)
                {
                    --movementintertia;
                    savemove = true;
                }
                tickmove = now;
            }
        }

        //
        // TIMELAPSE
        //
        if(GCFG->_glb.timelapse > 0)
        {
            if((now - lapsetick) > (uint32_t)GCFG->_glb.timelapse)
            {
                savelapse = true;
                lapsetick = now;
            }
        }

        //
        // DARK DISABLE TIMELAPSE
        //
        if(dev.darkaverage() < (uint32_t)GCFG->_glb.darklapse)
        {
            savelapse = false;
        }
        if(dev.darkaverage() < (uint32_t)GCFG->_glb.darkmotion)
        {
            savemove = false;
        }


        //
        // SAVE FILES SEQUENCIALLY ON DRIVE
        //
        if(!pathname.empty() && (savelapse || savemove || GCFG->_glb.oneshot || _sig_proc_capture) )
        {
            char fname[256];
            ::sprintf(fname, "%si%04d-%06d.jpg", pathname.c_str(), movepix, firstimage);
            ++firstimage;
            if(firstimage > maxfiles)  firstimage = 0;
            FILE* 		pff = ::fopen(fname,"wb");
            if(pff)
            {
                ::fwrite(pjpg,1,jpgsz,pff);
                ::fclose(pff);
                std::cout << "saving: " << fname << "\n";
                ::symlink(fname,"tmp/lastimage.jpg");
                if(GCFG->_glb.userpid > 0)
                {
                    ::kill(GCFG->_glb.userpid, SIGUSR2);
                    std::cout << "SIGUSR2: " << GCFG->_glb.userpid << "\n";
                }
            }
        }
        savelapse = false;
        savemove = false;
        movepix = 0;
        _sig_proc_capture = false;
        if(GCFG->_glb.oneshot)
            break;
    }

    cast.stop_thread();
}
