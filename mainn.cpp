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
#include "camevents.h"
/*
sudo apt-get install libv4l-dev libjpeg-dev
*/

using namespace std;
bool            __alive = true;

static void ControlC (int i)
{
    (void)i;
    __alive = false;
    printf("Exiting...\n");
}

static void Capture (int i)
{
    (void)i;
}


static void kapture();


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
    imglayout_t             image;
    std::string             lib_av      = CFG["image"]["lib_av"].value();
    std::string             lib_curl    = CFG["image"]["lib_curl"].value();
    int                     jquality    = CFG["image"]["jpg_quality"].to_int();
    EIMG_FMT                img_format  = (EIMG_FMT)CFG["image"]["img_format"].to_int();
    dims_t                  img_size    = CFG["image"]["img_size"].to_dims();
    bool                    bw_image    = CFG["image"]["bw_image"].to_int()!=0;
    event_t                 loopevent;
    std::vector<acamera*>   cameras;
    sockserver*             pserver     = nullptr;
    encoder*                pencoder    = nullptr;

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
#ifdef WITH_RTSP
            if(url.find("rtsp")!=(size_t)-1)
                pc = new rtspcam(img_size, name, url, pd);
            else
#endif
                if(url.find("http")!=(size_t)-1)
                    pc = new httpcam(img_size, name, url, pd);
                else
                    TRACE() << "NO CAM CONFIGURED\n";

        }
        else if(!dev.empty())
        {
            pc = new localcam(img_size, name, dev, pd);
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
            pserver->spin();
        }
        for(auto &ap : cameras)
        {
            acamera* p = ap;
            image._caml = p->get_frame(&image._camp, image._camf);
            if(image._caml)
            {
                if(image._camf == e422)
                {
                    image._jpgl = pencoder->fmt42_to_jpg(image._camp,
                                                     img_size.x,
                                                     img_size.y,
                                                     &image._jpgp);
                    image._jpgf = eFJPG;
                }
                else
                {
                    image._jpgf = image._camf;
                    image._jpgp = image._camp;
                    image._jpgl = image._caml;
                }
                loopevent = p->get_flag();
                p->proc_events(image);
                if(p->peer())
                {
                    p->peer()->stream(image._jpgp, image._jpgl, img_size, p->name(), loopevent, image._jpgf);
                }
                if(pserver && pserver->has_clients(p->name()))
                {
                    int wants = pserver->anyone_needs();
                    if(wants & ~WANTS_MOTION)
                    {
                        pserver->stream_on(image._jpgp, image._jpgl, image._jpgf, wants);
                    }
                    else
                    {
                        int             w,h,sz;
                        const uint8_t*  mot = p->getm(w, h, sz);
                        if(mot)
                        {
                            const uint8_t* pjpg = 0;
                            int jpgsz = pencoder->fmt42_to_bw(mot, w, h, &pjpg);
                            if(jpgsz){
                                pserver->stream_on(pjpg, jpgsz, eFJPG, wants);
                            }
                        }else{
                            pserver->stream_on(nullptr, 0, eFJPG, wants);
                        }
                    }
                }
                p->clean_events();
            }
        }
        ::usleep(16000);
    }

    delete pserver;
    for(auto& a: cameras){
        delete a;
    }
    delete pencoder;
}

