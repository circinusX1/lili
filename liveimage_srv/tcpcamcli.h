///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef TCPCAMCLI_H
#define TCPCAMCLI_H

#include <set>
#include "rawsock.h"
#include "fpipe.h"
#include "encrypter.h"

class TcpCamCli;
class TcpWebSock;


class TcpCamCli : public RawSock
{
public:
    TcpCamCli(RawSock& ,
            const LiFrmHdr& h,const char* pextra);
    virtual ~TcpCamCli();
    virtual void bind(TcpWebSock* b,bool addremove);
    virtual int  transfer(const std::vector<RawSock*>& clis);
    virtual bool destroy(bool be=true);
    virtual void can_send(bool force=false);

protected:
    int       _deliverChunk(const uint8_t* vf,int rec_off);
    size_t    _deliverFrame(int);
    void    _save_seq(){
        char fn[256];
        ::snprintf(fn, 255,"/tmp/%s.seq" ,_name.c_str());
        FILE *pf = ::fopen(fn,"wb");
        if(pf){
            ::fwrite(&_seq,1,sizeof(_seq),pf);
            ::fclose(pf);
        }
    }
    void   _load_seq(){
        char fn[256];
        _seq = 0;
        ::snprintf(fn, 255,"/tmp/%s.seq" ,_name.c_str());
        FILE *pf = ::fopen(fn,"rb");
        if(pf){
            ::fread(&_seq,1,sizeof(_seq),pf);
            ::fclose(pf);
        }
    }
protected:
    Fpip*       _pfpipe = nullptr;
    std::set<TcpWebSock*>  _pclis;
    size_t      _cap = 0;
    size_t      _byteson=0;
    size_t      _seps=0;
    bool        _asis=false;
    uint32_t    _rtpseq=0;
    std::string _recordname;
    bool        _pipefile=false;
    size_t      _bps;
    time_t      _fpst;
    int         _ask_frame=1;
    time_t      _lastask = 0;
    std::string _on_conhdr;
    int         _maxseq = 8912;
    std::string _onmaxseq;
    std::string _fpath;
};

#endif // TCPCAMCLI_H
