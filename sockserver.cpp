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

#include <iostream>
#include <fcntl.h>
#include "sockserver.h"
#include "encoder.h"
#include "cbconf.h"

#include "strutils.h"

extern bool __alive;

sockserver::sockserver(int port, const dims_t& d, EIMG_FMT fmt):_port(port)
{
    _ifmt = fmt;
    _img_size.x = d.x;
    _img_size.y = d.y;
    if(_ifmt==eFJPG){
        _mime="image/jpeg";
    }else{
        _mime="video/mpeg";
    }
    _mpg_multi_part = CFG["server"]["mpeg_mpart"].to_int()==1;
}

sockserver::~sockserver()
{
    close();
}

bool sockserver::listen()
{
    int ntry = 0;
AGAIN:
    if(__alive==false)
        return false;
    if(_s.create(_port, SO_REUSEADDR, 0)>0)
    {
        fcntl(_s.socket(), F_SETFD, FD_CLOEXEC);
        //_s.set_blocking(0);
        if(_s.listen(4)!=0)
        {
            TRACE() <<"socket can't listen. Trying "<< (ntry+1) << " out of 10 \n" ;

            _s.destroy();
            sleep(1);
            if(++ntry<10)
                goto AGAIN;
            return false;
        }
        TRACE() << "listening port"<< _port<<"\n";
        return true;
    }
    TRACE() <<"create socket. Trying "<< (ntry+1) << " out of 10 \n";
    _s.destroy();
    sleep(2);
    if(++ntry<10)
        goto AGAIN;
    __alive=false;
    sleep(3);
    return false;
}

void sockserver::close()
{
    _s.destroy();
    for(auto& s : _clis)
    {
        delete s;
    }
}

void sockserver::_check_and_keep(imgclient* pcli)
{
    imgclient* pcliis = nullptr;
    size_t     index = 0;

    for(auto& s : _clis)
    {
        if(!::strcmp(s->ssock_addrip(),pcli->ssock_addrip()))
        {
            pcliis = s;
            break;
        }
        ++index;
    }
    if(pcliis){
        pcliis->destroy();
        _clis[index] = pcli;
    }else{
        _clis.push_back(pcli);
    }
}

bool sockserver::spin(event_t&)
{
    fd_set  rd;
    int     ndfs = _s.socket();// _s.sock()+1;
    timeval tv {0,10000};

    FD_ZERO(&rd);
    FD_SET(_s.socket(), &rd);
    for(auto& s : _clis)
    {
        if(s->socket()>0)
        {
            FD_SET(s->socket(), &rd);
            ndfs = std::max(ndfs, s->socket());
        }
        else
            _dirty = true;
    }
    int is = ::select(ndfs+1, &rd, 0, 0, &tv);
    if(is ==-1) {
        TRACE() << "socket select() \n";
        __alive=false;
        return false;
    }
    if(is)
    {
        if(FD_ISSET(_s.socket(), &rd))
        {
            imgclient* cs = new imgclient();
            if(_s.accept(*cs)>0)
            {
                cs->_needs=0;
                TRACE() <<"\r\n\r\n-------------------\r\nnew connection \n";
                // do we have a client form same ip ?
                _check_and_keep(cs);
            }
        }

        for(auto& s : _clis)
        {
            if(s->socket()<=0)
                continue;
            if(FD_ISSET(s->socket(), &rd))
            {
                char req[512] = {0};

                int rt = s->receive(req,511);
                if(rt==0)//con closed
                {
                    TRACE() << "client closed connection \r\n";
                    s->destroy();
                    _dirty = true;
                }
                if(rt > 0)
                {
                    TRACE() << "REQUEST[" << req << "]\n";
                    char * host = strstr(req, "Host:");
                    if(host){
                        char* eol = strstr(host,"\r\n");
                        if(eol){
                            *eol=0;
                            _host = host+6;
                        }
                    }

                    if(s->_needs == 0 )
                    {
                        int iform = _ifmt;
                        for(const auto& a : _cams)
                        {
                            if(strstr(req,a.c_str()))
                            {
                                s->_camname = a;
                                break;
                            }
                        }

                        if( strstr(req, "motion") && iform == 0)
                        {
                            s->_needs = WANTS_MOTION;
                        }
                        else if( strstr(req, "stream"))
                        {
                            s->_needs = WANTS_LIVE_IMAGE;
                        }
                        else if( strstr(req, "html"))
                        {
                            s->_needs = WANTS_HTML;
                        }
                        else
                        {
                            std::string r = "\r\n<H1>LIVEIMAGE</H1>"
                                            "<li><b>Image format is: ";
                            r += std::to_string(iform);
                            r += "</b>";
                            for(const auto& a : _cams)
                            {
                                r+="<li>Camera: ";
                                r+= a;
                                r+="<ul>";
                                r+="<li>JPEG: <a href='http://";
                                r+=_host;r+="/?";r+=a; r+="&stream'>STREAM</a>";
                                r+= "<li>JPEG <a href='http://";
                                r+=_host;r+="/?";r+=a; r+="&motion'>MOTION</a>";
                                r+= "<li>MPEG <a href='http://";
                                r+=_host;r+="/?";r+=a;r+="&html'>HTML</a>";
                                r+="</ul>";
                            }
                            int lrsp = ::snprintf(req,sizeof(req),HEADER_200,(int)r.length());
                            s->send(req,lrsp);
                            s->send(r.c_str(), r.length());
                            s->destroy();
                            _dirty = true;
                            TRACE() << "RSPONSE[" << r << "]\n";
                        }
                    }

                }
            }
        }
    }
    _clean();

    return _dirty;
}

bool sockserver::has_clients(const std::string& camname)
{
    for(const auto& a : _clis)
    {
        if(a->_camname==camname){
            return true;
        }
    }
    return false;
}


int  sockserver::anyone_needs()const
{
    int needs = 0;
    for(auto& s : _clis)
        needs |= s->_needs;
    return needs;
}

void sockserver::reg_cam(const std::string& camname)
{
    _cams.push_back(camname);
}


void sockserver::_clean()
{
    if(_dirty)
    {
AGAIN:
        for(std::vector<imgclient*>::iterator s=_clis.begin();s!=_clis.end();++s)
        {
            if((*s)->socket()<=0)
            {
                delete (*s);
                _clis.erase(s);
                TRACE() << " client gone \n";
                goto AGAIN;
            }
        }
    }
    _dirty=false;
}

void sockserver::_send_page(imgclient* pc, int ifmt)
{
    char  image[512];
    char  html[1024];
    int len;
    if(ifmt==0)
    {
        len = ::sprintf(image,
                        "<img width='640' src='http://%s/?stream&%s' />",_host.c_str(), pc->_camname.c_str());
    }
    else
    {
        len = ::sprintf(image, "<video id='player' width='%d' height='%d' "
                               "controls>"
                               "<src='http://%s/?stream&%s' type='%s'>"
                               "<p>This is fallback content</p>"
                               "</video>\r\n", _img_size.x,
                        _img_size.y,
                        _host.c_str(),
                        pc->_camname.c_str(),
                        _mime.c_str());

    }

    len = ::sprintf(html,
                    "HTTP/1.1 200 OK\r\n"
                    "Content-length: %d\r\n"
                    "Content-Type: text/html\r\n\r\n%s",len, image);
    pc->sendall(html, len);

    TRACE() << "serving page [" << html << "]\r\n";
}

bool sockserver::stream_on(const uint8_t* buff, uint32_t sz, int ifmt, int wants)
{
    bool rv;

    for(auto& s : _clis)
    {
        switch(s->_needs)
        {
        case WANTS_MOTION:
            if(wants == WANTS_MOTION && ifmt==0)
                rv = this->_stream_jpeg(s, buff, sz);
            break;
        case WANTS_LIVE_IMAGE:
            if((wants == WANTS_LIVE_IMAGE && ifmt==0) ||  _mpg_multi_part) // streams mpg as jpg
                rv = this->_stream_jpeg(s, buff, sz);
            else
                rv = this->_stream_video(s, buff, sz);
            break;
        case WANTS_HTML:
            _send_page(s, ifmt);
            s->destroy();
            _dirty = true;
            break;
        default:
            break;
        }
    }
    if(_dirty)
        _clean();
    return rv;
}

bool sockserver::_stream_jpeg(imgclient* pc, const uint8_t* buff,
                              uint32_t sz)
{
    char buffer[256] = {0};
    struct timeval timestamp;
    struct timezone tz = {5,0};
    int rv = 0;

    gettimeofday(&timestamp, &tz);
    if(!pc->_headered)
    {
        TRACE() << "HEADERING\r\n";
        rv = pc->sendall(HEADER_JPG, strlen(HEADER_JPG),100);
        if(rv!=strlen(HEADER_JPG))
        {
            pc->destroy();
            _dirty=true;
            return false;
        }
        pc->_headered=true;
        msleep(0x1f);
    }
    sprintf(buffer, "Content-Type: %s\r\n" \
                    "Content-Length: %d\r\n" \
                    "Connection: close\r\n" \
                    "X-Timestamp: %d.%06d\r\n" \
                    "\r\n",
            _mime.c_str(),
            sz,
            (int)timestamp.tv_sec, (int)timestamp.tv_usec);
    rv = pc->sendall(buffer,strlen(buffer),100);
    if(rv==0)
    {
        pc->destroy();
        _dirty=true;
        pc->_headered = false;
        return false;
    }
    rv = pc->sendall(buff,sz,1000);
    if(rv==0)
    {
        pc->destroy();
        _dirty=true;
        pc->_headered = false;
        return false;
    }
    ::sprintf(buffer, "\r\n--MY_BOUNDARY_STRING_NOONE_HAS\r\nContent-type: imagejpg\r\n");
    rv = pc->sendall(buffer,strlen(buffer),100);
    if(rv==0)
    {
        pc->destroy();
        pc->_headered=false;
        _dirty=true;
    }
    return true;
}

bool sockserver::_stream_video(imgclient* pc, const uint8_t* buff, uint32_t sz)
{
    int rv = 0;
    rv = pc->sendall(buff,sz,1000);
    if(rv==0)
    {
        pc->destroy();
        pc->_headered=false;
        _dirty=true;
        return false;
    }
    TRACE() << "streaming mpeg " << rv << "\r\n";
    return true;
}

bool sockserver::init(const dims_t&)
{
    return listen();
}
