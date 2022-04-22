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
#include "sockserver.h"
#include "jpeger.h"
#include "mpeger.h"
#include "cbconf.h"
#include "rtspcam.h"
#include "localcam.h"
#include "httpcam.h"
#include "webcast.h"
#include "lilitypes.h"
#include "motion_track.h"
/*
sudo apt-get install libv4l-dev libjpeg-dev
*/

using namespace std;
bool            __alive = true;
bool            _sig_proc_capture=false;

struct  MotionLapse{
    int          robinserve     = 0x1;
    time_t       mpgnewfile     = 0;
    time_t       lapsetick      =  gtc();
    time_t       tickmove       =  gtc();
    event_t      event          = {0,0};
    dims_t       mohilo         = CFG["move"]["motion"].to_dims();
    int          innertia       = CFG["move"]["innertia"].to_int();
    int          inertiiaitl    = CFG["move"]["inertiiaitl"].to_int();
    int          time_lapse     = CFG["move"]["time_lapse"].to_int();
    int          dark_lapse     = CFG["move"]["dark_lapse"].to_int();
    int          dark_motion    = CFG["move"]["dark_motion"].to_int();
    int          one_shot       = CFG["move"]["one_shot"].to_int();
    std::string  save_loc       = CFG["record"]["save_loc"].value();
    int          max_size       = CFG["record"]["max_size"].to_int();
    int          on_max_files   = CFG["record"]["on_max_files"].to_int(0);
    std::string  on_max_comand  = CFG["record"]["on_max_files"].value(1);
    int          sig_process    = CFG["move"]["sig_process"].to_int();
    int          movementintertia = 0;
    int          firstimage = 0;
};


static void ControlC (int i)
{
    (void)i;
    __alive = false;
    printf("Exiting...\n");
}

static void Capture (int i)
{
    (void)i;
    _sig_proc_capture=true;
}


static void kapture();
void motion_lapse(motion_track* mt, MotionLapse*, const uint8_t*, const uint8_t* jpgimg,
                  size_t jpgl,  EIMG_FMT, EIMG_FMT);


int main(int nargs, char* vargs[])
{

    (void)nargs;
    (void)vargs;

    signal(SIGABRT, ControlC);
    signal(SIGKILL, ControlC);
    signal(SIGUSR2, Capture);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGPIPE);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);

    pCFG = new Cbdler();
    try{
        pCFG->parse("./liveimage.konf");
        kapture();
    }catch(int line)
    {
        TRACE() << "\n" << Cbdler::_last_line <<":"<< Cbdler::_ilast_line<<"\n";
    }
    delete pCFG;
}

////////////////////////////////////////////////////////////////////////////////////////////////
void kapture()
{

    std::string             lib_av      = CFG["image"]["lib_av"].value();
    std::string             lib_curl    = CFG["image"]["lib_curl"].value();
    int                     jquality    = CFG["image"]["jpg_quality"].to_int();
    EIMG_FMT                img_format  = (EIMG_FMT)CFG["image"]["img_format"].to_int();
    dims_t                  img_size    = CFG["image"]["img_size"].to_dims();
    bool                    bw_image    = CFG["image"]["bw_image"].to_int()!=0;
    dims_t                  mohilo      = CFG["move"]["motion"].to_dims();

    sockserver*             pserver     = nullptr;
    MotionLapse             mlapse;
    std::vector<acamera*>   cameras;
    motion_track*           pmotion     = nullptr;
    encoder*                pencoder    = nullptr;
    size_t                  frmlen;
    int                     jpgsz;
    const uint8_t*          frmdata;
    const uint8_t*          pjpg;
    EIMG_FMT                fmt = e422;
    EIMG_FMT                camfmt = e422;

    if(mohilo.y > mohilo.x){
        pmotion = new motion_track(mohilo.x, mohilo.y, img_size);
    }
    int local_server_port = CFG["server"]["port"].to_int();
    if(local_server_port){
        pserver = new sockserver(local_server_port, img_size, img_format);
        if(!pserver->init(img_size))
        {
            TRACE() << "failed to start server\n";
            delete pserver;
            pserver = nullptr;
        }
    }


    size_t cams = CFG["cameras"].count();
    for(size_t c = 0 ; c < cams; c++)
    {
        event_t flag ={0,0};
        const Cbdler::Node& pd      = CFG["cameras"].scope(c);
        const std::string& name     = pd["name"].value();
        const std::string& url      = pd["url"].value();
        const std::string& dev      = pd["device"].value();

        if(name.empty() || name.at(0)=='~')
            continue;

        const Cbdler::Node& nev = pd["on_event"];
        for(size_t ev=0;ev<nev.count();ev++){
            if(nev.value(ev)=="record"){
                flag.predicate |= FLAG_RECORD;  // @SERVER
            }
            else if(nev.value(ev)=="webcast"){
                flag.predicate |= FLAG_WEBCAST;
            }
            else if(nev.value(ev)=="save"){
                flag.predicate |= FLAG_SAVE;
            }
            else if(nev.value(ev)=="force"){
                flag.predicate |= FLAG_FORCE_SAVE;
            }
        }

        acamera* pc;
        if(!url.empty())
        {
            if(url.find("rtsp")!=(size_t)-1)
                pc = new rtspcam(name, url, pd);
            if(url.find("http")!=(size_t)-1)
                pc = new httpcam(name, url, pd);

        }
        else if(!dev.empty())
        {
            pc = new localcam(name, dev, pd);
        }
        pc->set_flag(flag);
        std::string swebcast = CFG["webcast"]["server"].value();
        if(!swebcast.empty())
        {
            imgsink* ps = new webcast(name);
            ps->init(img_size);
            pc->set_peer(ps);
        }
        pc->init(img_size);
        cameras.push_back(pc);
        if(pserver)
            pserver->reg_cam(pc->name());
    }

    if(img_format == eFJPG){
        pencoder = new jpeger(jquality, bw_image);
    }else{
        pencoder = new mpeger(jquality, bw_image);
    }

    while(__alive && 0 == ::usleep(10000))
    {
        if(pserver){
            pserver->spin(mlapse.event);
        }
        for(auto &p : cameras)
        {
            p->spin(mlapse.event);
            frmlen = p->get_frame(&frmdata, fmt, mlapse.event);
            camfmt = fmt;
            if(frmlen)
            {
                mlapse.event = p->get_flag();
                if(fmt == e422)
                {
                    jpgsz = pencoder->convert420(frmdata, frmlen, img_size.x, img_size.y, &pjpg);
                    fmt = eFJPG;
                }
                else
                {
                    pjpg = frmdata;
                    jpgsz = frmlen;
                }
                if(pmotion){
                    mlapse.event = p->get_flag();
                    motion_lapse(pmotion, &mlapse, frmdata, pjpg, jpgsz, fmt, camfmt);
                }
                if(p->peer()){
                    p->peer()->stream(pjpg, jpgsz, img_size, p->name(), mlapse.event, fmt);
                }
                if(pserver && pserver->has_clients(p->name()))
                {
                    int wants = pserver->anyone_needs();
                    if(wants & ~WANTS_MOTION){
                        pserver->stream_on(pjpg, jpgsz, fmt, wants);
                    }
                    else if(pmotion)
                    {
                        int             w,h,sz;
                        const uint8_t*  mot = pmotion->getm(w, h, sz);
                        jpgsz = pencoder->convertBW(mot, sz, w, h, &pjpg);
                        pserver->stream_on(pjpg, sz, fmt, wants);
                    }
                }
                mlapse.event.movepix = 0;
                mlapse.event.predicate = 0;
                _sig_proc_capture = false;
            }
        }

        if(mlapse.one_shot)
            break;
    }

    delete pserver;
    for(auto& a: cameras){
        delete a;
    }
    delete pmotion;
    delete pencoder;
}

void motion_lapse(motion_track* mt,
                  MotionLapse* pm,
                  const uint8_t* frmdata,
                  const uint8_t* jpgimg,
                  size_t  jpgl,
                  EIMG_FMT fmt, EIMG_FMT camfmt)
{
    char fname[256];
    uint32_t now =  gtc();

    if(now - pm->tickmove > pm->inertiiaitl)
    {
        pm->event.movepix = mt->movement(frmdata, camfmt);
        if( pm->event.movepix )
        {
            pm->movementintertia = pm->innertia;
        }
        if(pm->movementintertia > 0)
        {
            --pm->movementintertia;
            if(pm->event.movepix==0){
                pm->event.movepix = pm->movementintertia;
            }
        }
        if(pm->event.movepix){
            pm->event.predicate |= EVT_MOTION;
        }
        pm->tickmove = now;
    }

    if(pm->time_lapse > 0)
    {
        if((now - pm->lapsetick) > (uint32_t)pm->time_lapse)
        {
            pm->event.predicate |= EVT_TLAPSE;
            pm->lapsetick = now;
        }
    }

    if(mt->darkaverage() < pm->dark_lapse)
    {
        pm->event.predicate &= ~EVT_TLAPSE;
        pm->event.predicate &= ~EVT_MOTION;
    }

    if(_sig_proc_capture){
        _sig_proc_capture=0;
        pm->event.predicate |= EVT_SIGNAL;
     }


    if((pm->event.predicate & FLAG_SAVE && pm->event.movepix) ||
       pm->event.predicate & FLAG_FORCE_SAVE)
    {
        if(fmt!=0)
        {
            if(time(0) - pm->mpgnewfile > 3600*24)  /*every day*/
            {
                pm->mpgnewfile = time(0);
                ::sprintf(fname, "%smov_%d.mpeg", pm->save_loc.c_str(), (int)pm->mpgnewfile);
                FILE* 		pff = ::fopen(fname,"wb");
                if(pff){
                    ::fclose(pff);
                }
            }
            else
            {
                ::sprintf(fname, "%smov_%d.mpeg", pm->save_loc.c_str(), int(pm->mpgnewfile));
                FILE* 		pff = ::fopen(fname,"ab");
                long        flen = 0;
                if(pff)
                {
                    ::fseek(pff,0,SEEK_END);
                    flen = ::ftell(pff);
                    ::fwrite(jpgimg,1,jpgl,pff);
                    ::fclose(pff);
                }
                if(flen > (long) (1000000 * pm->max_size)){
                    pm->mpgnewfile = 0;          /*every exceed*/
                }
            }
        }
        else
        {
            ::sprintf(fname, "%si%04d-%06d.jpg", pm->save_loc.c_str(),pm->event.movepix, pm->firstimage);
            ++pm->firstimage;
            if(pm->firstimage > pm->on_max_files)  pm->firstimage = 0;
            FILE* 		pff = ::fopen(fname,"wb");
            if(pff)
            {
                ::fwrite(jpgimg,1,jpgl,pff);
                ::fclose(pff);
                TRACE() << "saving: " << fname << "\n";
                ::symlink(fname,"tmp/lastimage.jpg");
                if(pm->sig_process > 0)
                {
                    ::kill(pm->sig_process, SIGUSR2);
                    TRACE() << "SIGUSR2: " << pm->sig_process << "\n";
                }
            }
        }
    }
}


