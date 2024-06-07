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
#include "mainn.h"
#include "v4ldevice.h"
#include "sockserver.h"
#include "jpeger.h"
#include "mpeger.h"
#include "cbconf.h"
#include "rtspcli.h"
#include "localcam.h"
#include "webcast.h"

/*
sudo apt-get install libpng-dev libv4l-dev libjpeg-dev
*/

using namespace std;
bool            __alive = true;
bool            _sig_proc_capture=false;
static time_t   _mpgnewfile = 0;

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


static void capture(outstrmfmt* ffmt, sockserver* ps,
                    v4ldevice& dev, std::string save_loc,
                    int firstimage, int max_files, const point_t& wh, int quality);
static void calc_room(const std::string& save_loc, int& curentfile,
                      uint64_t& max_files, const point_t& imgsz);


int main(int nargs, char* vargs[])
{
    signal(SIGABRT, ControlC);
    signal(SIGKILL, ControlC);
    signal(SIGUSR2, Capture);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGPIPE);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

    (void)nargs;
    (void)vargs;

    pCFG = new Cbdler();
    try{
        pCFG->parse("./liveimage.konf");
    }catch(int line)
    {
        std::cerr << "\n" << Cbdler::_last_line <<":"<< Cbdler::_ilast_line<<"\n";
        exit(line);
    }

    point_t     wh      = CFG["I"]["img_size"].to_point();
    point_t     mo      = CFG["move"]["motion"].to_point();
    int         quality = CFG["I"]["jpg_quality"].to_int();
    int         ifmt    = CFG["I"]["img_format"].to_int();

    v4ldevice   dev(CFG["I"]["device"].to_string().c_str(),
                    wh.x,
                    wh.y,
                    CFG["I"]["fps"].to_int(),
                    mo.x,
                    mo.y,
                    CFG["move"]["noise_div"].to_int());
    if(dev.open())
    {
        std::cout << CFG["I"]["device"].value() << " opened\n";
        sockserver* ps = 0;
        if(CFG["server"]["port"].to_int())
        {
            ps = new sockserver(CFG["server"]["port"].to_int(), "http");
            if(ps && ps->listen()==false)
            {
                delete ps;
                return 0;
            }
        }

        outstrmfmt*  ffmt = 0;

        if(ifmt!=0){
            ffmt = new mpeger(quality);
            if(ffmt->init(wh.x, wh.y)==false)
            {
                std::cerr << "cannot do mpeger \r\n. Using default jpeger\r\n";
                delete ffmt;
                ffmt = new jpeger(quality);
            }
        }
        else
            ffmt = new jpeger(quality);

        uint64_t max_files=0;
        uint32_t maxfiles2=0;
        int firstimage=0;
        if(CFG["I"]["max_files"].to_int()==0)
        {
            if(quality)
            {
                calc_room(CFG["I"]["save_loc"].value().c_str(),
                            firstimage,
                            max_files, wh);
            }
            maxfiles2=(uint32_t)max_files;
        }
        else
        {
            maxfiles2 = CFG["I"]["max_files"].to_int();
        }
        std::cout << "rotating images at:" << maxfiles2 << "\n";
        capture(ffmt, ps, dev,
                CFG["I"]["save_loc"].value(),
                firstimage, maxfiles2, wh, quality);

        delete ps;
        dev.close();
        delete ffmt;
    }
    delete pCFG;
}



void calc_room(const std::string& save_loc,
               int& firstimage,
               uint64_t& maximages,
               const point_t& iimgsz)
{
    struct statvfs64 fiData;

    if((statvfs64(save_loc.c_str(), &fiData)) == 0 )
    {
        uint64_t bytesfree = (uint64_t)(fiData.f_bfree * fiData.f_bsize);
        uint64_t imgsz = (uint64_t)_imagesz(iimgsz.x, iimgsz.y);

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
        char index[32];
        ::fgets(index, 7, pff);
        firstimage=::atoi(index);
        ::fclose(pff);
    }
    else
        firstimage = 0;
    std::cout << "Current image:" << firstimage << ", Roll up at:" << maximages << "\n";
}


void capture(outstrmfmt* ffmt, sockserver* ps, v4ldevice& dev,
             std::string save_loc, int firstimage,
             int max_files, const point_t& wh, int quality)
{
    uint32_t        jpgsz = 0;
    uint8_t*        pjpg = 0;
    const uint8_t*  pb422;
    int             robinserve = 0x1;
    time_t          lapsetick =  gtc();
    time_t          tickmove =  gtc();
    int             event = 0;
    bool            savemove = false;
    bool            savelapse = false;
    int             movementintertia = 0;
    int             sz = 0;
    int             iw = wh.x;
    int             ih = wh.y;
    std::vector<rtspcli*>           remotes;
    WebCast         *cast            = nullptr;
    bool            fatal            = false;
    int             ifmt             = CFG["I"]["img_format"].to_int();
    int             wecasting       = CFG["webcast"]["enabled"].to_int();
    point_t         motion           = CFG["move"]["motion"].to_point();
    int             innertia         = CFG["move"]["innertia"].to_int();
    int             innertia_interval= CFG["move"]["innertia_interval"].to_int();
    int             time_lapse       = CFG["move"]["time_lapse"].to_int();
    int             one_shot         = CFG["move"]["one_shot"].to_int();
    int             dark_lapse       = CFG["move"]["dark_lapse"].to_int();
    int             dark_motion      = CFG["move"]["dark_motion"].to_int();
    int             max_size         = CFG["I"]["max_size"].to_int();
    int             sig_process      = CFG["move"]["sig_process"].to_int();


    _mpgnewfile = time(0);

    size_t cams = CFG["remote"].count();
    for(size_t c = 0 ; c< cams; c++)
    {
        const Cbdler::Node& pd = CFG["remote"].blob(c);
        const std::string& name = pd["name"].value();
        /*
        SETUP rtsp://192.168.10.93:1234/trackID=1 RTSP/1.0
        CSeq: 3
        Transport: RTP/AVP;unicast;client_port=8000-8001
        */

        const std::string& transport ="RTP/AVP;unicast;client_port=";
        const std::string& url  = pd["url"].value();
        const std::string  usr  = pd["user"].value();
        const std::string  pass  = pd["pass"].value();
        rtspcli* pc = new rtspcli(name.c_str(), url.c_str(),
                                  transport.c_str(),
                                  usr.c_str(),
                                  pass.c_str());
        remotes.push_back(pc);
    }

    if(remotes.size())
    {
        for(auto& a : remotes)
        {
            a->start_thread();
        }
    }

    if(wecasting)
    {
        cast = new WebCast(ifmt);
        cast->start_thread();
    }

    while(__alive && 0 == ::usleep(10000))
    {
        if(ps)
            ps->spin();
        fatal =false;
        pb422 = dev.read(iw, ih, sz, fatal);
        if(pb422 == 0){
            if(fatal){
                std::cout << "fatal error io. exiting \n";
                __alive=false; //let the service script handle it
            }
            ::sleep(1);
            continue;
        }
        jpgsz = ffmt->convert420(pb422, sz, iw, ih, quality, &pjpg);
        if(ps && ps->has_clients() && jpgsz)
        {
            int wants = ps->anyone_needs();
            if(wants == WANTS_HTML)
            {
                ps->stream_on(0, 0, ifmt, WANTS_HTML);
            }
            else if(wants & WANTS_LIVE_IMAGE && robinserve  ==  WANTS_LIVE_IMAGE)
            {
                ps->stream_on(pjpg, jpgsz,ifmt, WANTS_LIVE_IMAGE);
            }
            else if(wants & WANTS_MOTION && robinserve  ==  WANTS_MOTION)
            {
                uint8_t*        pjpg1 = 0;
                int             w,h;
                const uint8_t*  mot = dev.getm(w, h, sz);
                size_t          jpgsz1 = ffmt->convertBW(mot,
                                                         sz, w, h, 80,
                                                         &pjpg1);
                if(jpgsz1)
                {
                    ps->stream_on(pjpg1, jpgsz1, ifmt, WANTS_MOTION);
                }
            }
            else if(remotes.size())
            {
                for(const auto& a : remotes)
                {
                    if(WANTS_REMOTE & wants)
                    {
                        jpgsz = a->getframe(&pjpg);
                        ps->stream_on(pjpg,jpgsz,ifmt, WANTS_REMOTE);
                    }
                }
            }
            robinserve <<= 0x1;
            if(robinserve==WANTS_MAX){
                robinserve = 0x1;
            }
        }

        //
        // MOTION
        //
        uint32_t now =  gtc();
        if(motion.x>0)
        {
            if(now - tickmove > innertia_interval)
            {
                event = dev.movement();
                if( event >= motion.x && event <= motion.y)
                {
                    //                    std::cout << "move pix=" << event << "\n";
                    savemove = true;
                    movementintertia = innertia;
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
        if(time_lapse > 0)
        {
            if((now - lapsetick) > (uint32_t)time_lapse)
            {
                savelapse = true;
                lapsetick = now;
            }
        }

        //
        // DARK DISABLE TIMELAPSE
        //
        if(dev.darkaverage() < dark_lapse)
        {
            savelapse = false;
        }
        if(dev.darkaverage() < dark_motion)
        {
            savemove = false;
        }

        if(cast && !cast->is_stopped())
        {
            cast->stream_frame(pjpg, jpgsz, event, iw, ih);
        }
        else{
            if(!save_loc.empty() && (savelapse || savemove ||
                                     one_shot || _sig_proc_capture) )
            {
                char fname[256];

                if(ifmt!=0)
                {
                    if(time(0) - _mpgnewfile > 3600)
                    {
                        _mpgnewfile = time(0);
                        ::sprintf(fname, "%smov_%d.mpeg", save_loc.c_str(), (int)_mpgnewfile);
                        FILE* 		pff = ::fopen(fname,"wb");
                        if(pff){
                            ::fclose(pff);
                        }
                        _mpgnewfile = time(0);
                    }
                    else
                    {
                        ::sprintf(fname, "%smov_%d.mpeg", save_loc.c_str(),
                                            int(_mpgnewfile));
                        FILE* 		pff = ::fopen(fname,"ab");
                        long        flen = 0;
                        if(pff)
                        {
                            ::fseek(pff,0,SEEK_END);
                            flen = ::ftell(pff);
                                    ::fwrite(pjpg,1,jpgsz,pff);
                            ::fclose(pff);
                        }
                        if(flen > (long) (1000000 * max_size)){
                            _mpgnewfile = 0;
                        }
                    }
                    //stream in the same file as long the time is less then 10 seconds
                    // as we come here
                }
                else
                {
                    ::sprintf(fname, "%si%04d-%06d.jpg", save_loc.c_str(),
                              event, firstimage);
                    ++firstimage;
                    if(firstimage > max_files)  firstimage = 0;
                    FILE* 		pff = ::fopen(fname,"wb");
                    if(pff)
                    {
                        ::fwrite(pjpg,1,jpgsz,pff);
                        ::fclose(pff);
                        std::cout << "saving: " << fname << "\n";
                        ::symlink(fname,"tmp/lastimage.jpg");
                        if(sig_process > 0)
                        {
                            ::kill(sig_process, SIGUSR2);
                            std::cout << "SIGUSR2: " << sig_process << "\n";
                        }
                    }
                }
            }
        }

        savelapse = false;
        savemove = false;
        event = 0;
        _sig_proc_capture = false;
        if(one_shot)
            break;
    }
    if(cast && !cast->is_stopped())
    {
        std::cout << "killing cast \r\n";
        cast->kill();
        cast->stop_thread();
        delete cast;
    }
    for(auto& a : remotes){
        a->stop_thread();
        delete a;
    }
}
