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
#ifndef SOCKSERVER_H
#define SOCKSERVER_H

#include <string>
#include <vector>
#include "sock.h"
#include "cbconf.h"
#include "cbconf.h"
#include "imgsink.h"
#include "lilitypes.h"

#define     WANTS_LIVE_IMAGE     0x1
#define     WANTS_MOTION    0x4
#define     WANTS_MAX       0x8
#define     WANTS_HTML      0x10
#define     WANTS_REMOTE    0x20

#define  HEADER_JPG "HTTP/1.0 200 OK\r\n" \
                "Connection: close\r\n"     \
                "Server: liveimage/1.0\r\n"   \
                "Cache-Control: no-cache\r\n"   \
                "Content-Type: multipart/x-mixed-replace;boundary=MY_BOUNDARY_STRING_NOONE_HAS\r\n" \
                "\r\n" \
                "--MY_BOUNDARY_STRING_NOONE_HAS\r\n"

#define  HEADER_200 "HTTP/1.0 200 OK\r\n" \
                "Connection: close\r\n"     \
                "Content-Length: %d\r\n" \
                "Server: liveimage/1.0\r\n"   \
                "Cache-Control: no-cache\r\n\r\n"


class imgclient : public tcp_cli_sock
{
public:

    imgclient():_needs(0),_headered(false){}
    ~imgclient(){}
    int          _needs = 0;
    bool         _headered = false;;
    std::string  _message;
    std::string  _camname;
};

class acamera;
class sockserver
{
public:
    sockserver(int port, const dims_t& d, EIMG_FMT fmt);
    virtual ~sockserver();

    bool spin(std::vector<acamera*>& cameras);
    bool init(const dims_t&);
    bool listen();
    void close();
    int  socket() {return _s.socket();}
    bool has_clients(const std::string& camname);
    bool stream_on(const uint8_t* buff, uint32_t sz, int ifmt, int wants, acamera* cam);
    int  anyone_needs()const;
    void reg_cam(const std::string& camname);


private:
    void _clean();
    bool _stream_jpeg(imgclient* pc, const uint8_t* buff, uint32_t sz);
    bool _stream_video(imgclient* pc, const uint8_t* buff, uint32_t sz);
    void _send_page(imgclient* pc, int ifmt, acamera* pcam);
    void _check_and_keep(imgclient* pcli);
    void _config(imgclient* cli, const char* req, std::vector<acamera*>& cameras);

    tcp_srv_sock _s;
    tcp_srv_sock _h;
    int      _port;
    std::vector<imgclient*> _clis;
    bool     _dirty;
    string   _host;
    string   _mime;
    int      _ifmt;
    dims_t  _img_size;
    bool     _mpg_multi_part=false;
    std::vector<std::string> _cams;

};

#endif // SOCKSERVER_H
