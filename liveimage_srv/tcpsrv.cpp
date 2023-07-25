///////////////////////////////////////////////////////////////////////////////////////////////
#include <unistd.h>
#include <fcntl.h>
#include "main.h"
#include "cliqueue.h"
#include "pool.h"
#include "tcpcamcli.h"
#include "tcpjpgmpartcam.h"
#include "tcpwebsock.h"
#include "tcpwebjpgcli.h"
#include "tcpsrv.h"
#include "requestparser.h"
#include "sheller.h"
#include "md5.h"

#define SECRET_CONTENT        "marius-marius:secret"
#define ENC_KEY               "thesecretke_yweuse"
#define MAC_LENGTH            12
/**
 * @brief HDR for listing the channels open to html page
 */
static char HDR[] = "HTTP/1.0 200 OK\r\n"
                    "Connection: close\r\n"
                    "Server: liveimgsrv/1.0\r\n"
                    "Cache-Control: no-store, no-cache, must-revalidate, pre-check=0, post-check=0, max-age=0\r\n"
                    "Pragma: no-cache\r\n"
                    "Expires: Mon, 3 Jan 2000 12:34:56 GMT\r\n"
                    "Content-Type: text/html\r\n"
                    "Content-length: %d\r\n"
                    "X-Timestamp: %d.%06d\r\n\r\n";
static const char* SRV = "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 3.2 Final//EN\""
                         "<HTML><BODY>LIVESERVER 1.0</BODY></HTML>"
                         "<sub style='float:right'>by Marius C. "
                         "<a href='https://github.com/circinusX1/liveimage'>liveimage</a></sub>";


extern Encryptor*   pENC;
/**
 * @brief TcpSrv::TcpSrv
 * @param p
 * @param q
 */
TcpSrv::TcpSrv(Pool& p, CliQ& q,Sheller* tm):_p(p),_q(q),_ptm(tm)
{
    // record_on   {motion,signal,timelapse,force}
}

/**
 * @brief TcpSrv::~TcpSrv
 */
TcpSrv::~TcpSrv()
{
    _cli.destroy();
    _cam.destroy();
}

/**
 * @brief TcpSrv::spin
 * @param cport
 * @param cliport
 * @return
 */
bool    TcpSrv::spin(int cport, int cliport, pfn_cb cb)
{
    _cport = cport;
    if(_creates(cport, cliport))
    {
        fd_set r,x;
        Wdog.reg();
        while(__alive)
        {
            ::msleep(16);
            int ndfs = _fd_set(r,x);
            if(_fd_check(r,x, ndfs)==0){
                GLOGE("recreating listeners");
                if(!_creates(cport, cliport)){
                    break;
                }
            }
            cb();
            _p.kill_times();
            Wdog.pet();
        }
    }
    return true;
}

/**
 * @brief TcpSrv::_creates
 * @return
 */
int   TcpSrv::_creates(int cport, int cliport)
{
    _cam.destroy();
    _cli.destroy();
    if(_cam.create(cport, SO_REUSEADDR, 0)>0)
    {
        fcntl(_cam.socket(), F_SETFD, FD_CLOEXEC);
        _cam.set_blocking(0);
        if(_cam.listen(32)!=0)
        {
            _cam.destroy();
            return 0;
        }
        GLOGI("listening port "<< cport);
    }
    if(_cli.create(cliport, SO_REUSEADDR, 0)>0)
    {
        fcntl(_cli.socket(), F_SETFD, FD_CLOEXEC);
        _cli.set_blocking(0);
        if(_cli.listen(4)!=0)
        {
            _cli.destroy();
            return 0;
        }
        GLOGI("listening port "<< cliport);
    }
    return 1;
}

/**
 * @brief TcpSrv::_fd_set
 * @param rd
 * @return
 */
int   TcpSrv::_fd_set(fd_set& rd, fd_set& rx)
{
    int ndfs = max(_cam.socket(),_cli.socket());
    FD_ZERO(&rd);
    FD_ZERO(&rx);
    FD_SET(_cam.socket(), &rd);
    FD_SET(_cli.socket(), &rd);
    FD_SET(_cam.socket(), &rx);
    FD_SET(_cli.socket(), &rx);
    return ndfs+1;
}

/**
 * @brief TcpSrv::_fd_check
 * @param rd
 * @param ndfs
 */
int TcpSrv::_fd_check(fd_set& rd, fd_set& rx, int ndfs)
{
    timeval tv {0,0xFFFF};
    int     is = ::select(ndfs, &rd, 0, &rx, &tv);
    int     rv = 1;

    if(is ==-1) {
        rv = 0;
        if(--_fatals==0){
            GLOGE("to many select network errors");
            __alive = false;
        }
        GLOGE("select network error");
    }
    if(is>0)
    {
        if(FD_ISSET(_cli.socket(),&rd))
            _on_cli();
        if(FD_ISSET(_cam.socket(),&rd))
            _on_cam();
        if(FD_ISSET(_cam.socket(),&rx))
        {
            _cam.destroy();
            _cli.destroy();
            rv = 0;
            GLOGE("exception listener  cam");
        }
        if(FD_ISSET(_cli.socket(),&rx))
        {
            _cli.destroy();
            _cam.destroy();
            rv = 0;
            GLOGE("exception listener cli");
        }
    }

    return rv;
}

/**
 * @brief TcpSrv::_on_cam, camm connects here every 2 seconds or so
 * @return
 */
bool    TcpSrv::_on_cam()
{
    RawSock     s;
    LiFrmHdr     hdr;
    char         mpart_start_hdr[512] ={0};
    const Cbdler::Node& accepted = CFG["cameras"];

    if(_cam.accept(s)>0)
    {
        msleep(1);
        try{
            s.set_blocking(1);
            int bytes = s.receive((uint8_t*)&hdr, sizeof(hdr)); //header string
            if(bytes != sizeof(hdr))
            {
                GLOGW("Cannot invalid header");
                throw RawSock::CAM;
            }
            do{ // if a http comes on camera port
                char* p = (char*)&hdr;
                std::string sp = std::to_string(_cport);
                if(*p && ::strstr(p, sp.c_str())){
                    HttpRequestParser   parser;
                    Request             req;
                    const char*         end = p + sizeof(hdr)-1;
                    p[sizeof(hdr)-1] = '\0';
                    parser.parse(req, p, end);
                    _show_dummy(s , req);
                    throw RawSock::CAM;
                }
            }while(0);

            if(pENC->decrypt(hdr.challange) != hdr.random)
            {
                GLOGW("invalid security: random=..." << hdr.random );
                throw RawSock::CAM;
            }

            TcpCamCli* cp = _p.has(hdr.camname);
            if(cp)
            {
                _p.kill_cam(hdr.camname);
                GLOGW("Conection refused: Already streaming from "<<hdr.camname);
                throw RawSock::CAM;
            }
            size_t checks = accepted.count();
            for(size_t i=0;i<accepted.count();i++){
                if(accepted.value(i)==hdr.camname){
                    break;
                }
                --checks;
            }

            if(checks==0){
                GLOGW("camera  ["<< hdr.camname<<"] not configured");
                throw RawSock::CAM;
            }
            // no client
            if(_p.client_was_here(std::string(hdr.camname))==false)
            {
                _p.record_cam(hdr.camname);
                if(! (hdr.event.predicate & EVT_KEEP_ALIVE))
                {
                    GLOGW("CAM:" << hdr.camname <<
                                " got header pred: KEEP_ALIV=0 " << hdr.index);
                    throw RawSock::CAM;
                }
                else
                {
                    GLOGW("CAM:" << hdr.camname <<  " event KEEP_ALIVE "  << hdr.index);
                }
            }
            GLOGW("camera  ["<< hdr.camname << "] ACCEPTED");

            RawSock* pcam = nullptr;
            if(hdr.format==0)
            {
                pcam = new TcpJpgMpartCam(s, hdr, mpart_start_hdr);
            }
            else
            {
                pcam = new TcpCamCli(s, hdr, mpart_start_hdr);
            }
            _q.push(pcam);
        }
        catch(RawSock::STYPE &t)
        {
            msleep(32);
            GLOGW("Closing cam connection");
            s.destroy();
            msleep(32);
        }
    }
    return true;
}

/**
 * @brief TcpSrv::_on_cli
 * @return
 */
bool    TcpSrv::_on_cli()
{
    RawSock s;

    if(_cli.accept(s)>0)
    {
        s.set_blocking(1);
        try{

            Request             req;
            HttpRequestParser   parser;
            char                hdr[2048];
            int                 bytes = s.select_receive((uint8_t*)hdr,
                                                         sizeof(hdr)-1,
                                                         10000,2);
            if(bytes<=0){
                return false;
            }
            hdr[bytes]=0;
            // GLOGD(">:" << hdr );
            const char* end = ::strstr(hdr,"\r\n\r\n");
            if(end==nullptr)end=(hdr + bytes);
            parser.parse(req, hdr,end);

            if(req.method=="GET" && req.uri.length()>2) //is a web
            {
                GLOGI("CLI CON [" << req.uri << "]");
                if(req.uri.at(1)=='?') // is a image link <img src='/?MAC'> We wnforce an image with a ?
                {
                    if(req.uri.substr(2) != __server_key)
                    {
                        _p.record_cli(req.uri.substr(2));
                    }
                    return _deal_cam_name(s, req, req.uri.substr(2));
                }
                else if(req.uri.at(1)=='!'  && req.uri.at(2)=='?'){
                    __alive=false;
                    throw RawSock::CLIENT;
                }
                return _show_streams(s, req, std::string(hdr)); //we dont know what it is
            }
            return _show_dummy(s, req);
        }
        catch(RawSock::STYPE &t)
        {
            GLOGW("Closing client connection");
            s.destroy();
        }
    }
    return true;
}

/**
 * @brief TcpSrv::_show_dummy
 * @param s
 */
bool    TcpSrv::_show_dummy(RawSock& s, const Request& re)
{
    std::string page=SRV;
    _htmlhdr(s, re, page.length());
    if(s.sendall(page.c_str(),page.length())!=int(page.length()))
    {
        throw RawSock::CLIENT;
    }
    return true;
}

/**
 * @brief TcpSrv::_show_streams
 * @param s
 * @param r
 * @return
 */
bool    TcpSrv::_show_streams(RawSock& s,
                              const Request& re,
                              const std::string& r)
{
    std::string channels=SRV;
    std::string host = re.headerAt("Host");
    if(!host.empty())
        channels = _p.get_cams(host, true);
    _htmlhdr(s,re,channels.length());
    s.snd((const uint8_t*)channels.c_str(),channels.length(),0);
    return true;
}

/**
 * @brief TcpSrv::_show_status
 * @param s
 * @param r  should be just the /isapi/channel the client wants
 * @return
 */
bool    TcpSrv::_deal_cam_name(RawSock& s,
                               const Request& htr,
                               const std::string& r)
{
    const TcpCamCli* pcs = _p.has(r);
    //is there a cam for this name ?
    if(pcs && !_p.has(s))           //one client for this stream
    {
        LiFrmHdr empty;
        ::strncpy(empty.camname, pcs->name().c_str(), sizeof(empty.camname));

        if(pcs->header()->format==0)
        {
            CliJpegSock* pcam = new CliJpegSock(s, empty);
            _q.push(pcam);
            return true;
        }
        else if(pcs->header()->format==0)
        {
            TcpWebSock* pcli = new TcpWebSock(s, empty);
            _q.push(pcli);
            return true;
        }
    }
    if(::strstr(r.c_str(),__server_key)){
        _zoomah = time(0);
        return _show_streams(s,htr, r);
    }
    else if(time(0) - _zoomah < 20){
        _zoomah = time(0);
        return _show_streams(s,htr, r);
    }
    if(_p.camera_kicking(r))
    {
        return _show_streams(s, htr, r);
    }
    return false;
}

/**
 * @brief TcpSrv::_show_status
 * @param s
 * @param r  should be just the /isapi/channel the client wants
 * @return
 */
bool    TcpSrv::_show_status(RawSock& s, const Request& re,
                             const std::string& r)
{
    const TcpCamCli* pcs = _p.has(r);
    if(_p.has(r) && !_p.has(s)) //one client for this stream
    {
        LiFrmHdr empty;

        ::strncpy(empty.camname, pcs->name().c_str(), sizeof(empty.camname));
        TcpWebSock* pcam = new TcpWebSock(s, empty);
        _q.push(pcam);
        return true;
    }
    return _show_streams(s,re, r);
}

void TcpSrv::_htmlhdr(RawSock& s, const Request& re, int length)
{
    char    hdr[512];
    struct  timeval timestamp;
    struct  timezone tz = {5,0};
    std::string host = re.headerAt("Host");

    gettimeofday(&timestamp, &tz);
    int len = (int)sprintf(hdr,HDR,  length,
                           (int) timestamp.tv_sec,
                           (int) timestamp.tv_usec);
    if(s.sendall(hdr, len) != len)
    {
        throw RawSock::CLIENT;
    }
}
