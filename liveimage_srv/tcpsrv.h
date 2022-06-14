///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TCPSRV_H
#define TCPSRV_H

#include <map>
#include <string>
#include "sock.h"
#include "os.h"

#define  LAST_CONTIME   12
typedef void (*pfn_cb)();

class RawSock;
class CliQ;
class Pool;
struct LiFrmHdr;
struct Request;
struct Sheller;
class TcpSrv
{
public:
    TcpSrv(Pool& p,CliQ& q,Sheller* ptm);
    ~TcpSrv();
    bool    spin(int cport,int cliport,pfn_cb cb);

private:
    int     _creates(int,int);
    int     _fd_set(fd_set&,fd_set&);
    int     _fd_check(fd_set&,fd_set&,int);
    bool    _on_cam();
    bool    _on_cli();
    bool    _show_streams(RawSock&,const Request& r,const std::string& cont);
    bool    _show_status(RawSock&,  const Request&,const std::string&);
    bool    _deal_cam_name(RawSock&,const Request& htr,const std::string&);
    bool    _show_dummy(RawSock&,const Request& htr);
    void    _htmlhdr(RawSock& s,const Request& re,int length);

    bool    _is_recent(const char* camname){
        auto a = _lastcons.find(camname);
        if(a !=_lastcons.end())
        {
            bool r = SECS()-a->second < LAST_CONTIME;
            a->second = SECS();
            return r;
        }
        _lastcons[camname]=SECS();
        return false;
    }
    void    _do_recent(){
        time_t now = SECS();
        AGAIN:
        for(auto& a : _lastcons){
            if(a.second - now > LAST_CONTIME){
                _lastcons.erase(a.first);
                goto AGAIN;
            }
        }
    }

private:
    Pool&           _p;
    CliQ&           _q;
    tcp_srv_sock    _cam;
    tcp_srv_sock    _cli;
    Sheller*        _ptm;
    std::map<std::string,time_t>   _lastcons;
    time_t          _zoomah = 0;
    int8_t          _fatals = 3;
    int             _cport=0;
};

#endif // TCPSRV_H
