///////////////////////////////////////////////////////////////////////////////////////////////
///
#include <sys/stat.h>
#include <sys/types.h>
#include "main.h"
#include "tcpcamcli.h"
#include "tcpwebsock.h"
#include "fpipe.h"
#include "sheller.h"

#define    MAX_JPG_FRM      65536
/**
 * @brief TcpCamCli::TcpCamCli
 * @param o
 * @param h
 */
TcpCamCli::TcpCamCli(RawSock& o,
                     const LiFrmHdr& h,const char* clionhdr):RawSock(o,h,RawSock::CAM),
    _on_conhdr(clionhdr)
{
    _cap = MAX_BUFF;
    _seps = 0;
    _name = _header.camname;

    if(_header.record)
    {
        time_t     now = SECS();
        char       buf[128];

        struct tm  tstruct = *localtime(&now);
        strftime(buf,sizeof(buf),"%s%Y-%m-%d.%X",&tstruct);
        _recordname = _header.camname;
    }
    _pipefile = false;  // TODO
    _asis = true;
    bind(nullptr,false);
    GLOGI("CAM CONNECTED "  << this->Rsin().c_str());
    _nowt = SECS();
    _bps = 0;
    _ask_frame = 1;

    _maxseq     = ::atoi(CFG["on_maxseq"].value(0).c_str());
    if(_maxseq == 0) _maxseq = 8912;
    _onmaxseq   = CFG["on_maxseq"].value(1);

    char    fnp[460];
    ::sprintf(fnp,"%s",__files_recs);
    if(::access(fnp,0)!=0)
    {
        ::mkdir(fnp,0777);
    }
    ::snprintf(fnp,sizeof(fnp)-1,"%s/%s",__files_recs,_recordname.c_str());
    if(::access(fnp,0)!=0)
    {
        ::mkdir(fnp,0777);
    }
    _fpath =fnp;

}

/**
 * @brief TcpCamCli::~TcpCamCli
 */
TcpCamCli::~TcpCamCli()
{
    destroy(true);
    GLOGI("CAM DIS_CONNECTED " << this->Rsin().c_str());
    GLOGD(__FUNCTION__);
}

/**
 * @brief TcpCamCli::destroy
 */
bool TcpCamCli::destroy(bool be)
{
    DELETE_PTR(_pf);
    return RawSock::destroy(be);
}

/**
 * @brief TcpCamCli::bind
 * @param pcs
 */
void TcpCamCli::bind(TcpWebSock* pcs,bool addremove)
{
    size_t  clis = _pclis.size();

    GLOGD("Camera" << _name << " has " << clis << " clients");
    for(const auto& a: _pclis){
        GLOGD(" client" << a->name());
    }

    if(addremove)
    {
        if(_pclis.find(pcs)==_pclis.end())
        {
            _pclis.insert(pcs);
        }
    }
    else
    {
        if(pcs==nullptr)
        {
            _pclis.clear();
        }
        else
        {
            if(_pclis.find(pcs)!=_pclis.end())
            {
                _pclis.erase(pcs);
            }
        }
    }
    DELETE_PTR(_pf);
    {
        if(_pipefile && _pf == nullptr)
        {
            _pf = new Fpip(_name);
        }
    }
}

/**
 * @brief TcpCamCli::can_send
 */
void TcpCamCli::can_send(bool force)
{
   if((_header.insync && tick_count()-_lastask>10000) || _ask_frame)
   {
        _header.mac = 888;
        this->snd((const uint8_t*)&_header,sizeof(_header),0);
        _ask_frame = 0;
        _lastask = tick_count();
    }
}

/**
 * @brief TcpCamCli::transfer
 * @param cli
 * @return
 */
int TcpCamCli::transfer(const std::vector<RawSock*>& clis)
{
    int bytes = this->recdata();
    if(bytes)
    {
        _deliverChunk(_data.buff,_data.rec_off);
    }
    _data.reset();
    _ask_frame = 1;
    return bytes;
}

/**
 * @brief TcpCamCli::_deliverChunk
 * @param vf
 */
int TcpCamCli::_deliverChunk(const uint8_t* vf,int rec_off)
{
    int  clients = 0;
    time_t now = SECS();

    _bps += rec_off;
    if(now > _fpst+5)
    {
        _fpst=now;
        GLOGI("Got: " << _bps/5 << " bps");
        _bps=0;
    }
    clients = _pclis.size();
    if(clients)
    {
        for(const auto& cs : _pclis)
        {
            if(cs->isopen()){
                cs->snd(vf,rec_off,_rtpseq);
            }
        }
    }
    else if(_pf)
    {
        _pf->stream(vf,rec_off);
    }

    if(!_recordname.empty() && _header.record)
    {
        FILE*   pf;
        char    fn[400];
        const uint8_t* bf = vf;

        snprintf(fn,sizeof(fn),"%s/%02d-%05zu.jpg",_fpath.c_str(),_header.event,_seq++);

        pf = ::fopen(fn,"wb");
        if(pf)
        {
            ::fwrite(bf,1,rec_off, pf);
            ::fclose(pf);
        }
        if((int)_seq > (int)_maxseq)
        {
            if(!_onmaxseq.empty())
            {
                std::string shell = "/bin/bash ";
                shell +=  _onmaxseq + " ";
                shell += _fpath; shell +=" ";
                shell += this->name() + " &";
                GLOGD("executing" << shell);
                ::system(shell.c_str());
                //_tm->run( namex,_onmaxseq,where,namex);
            }
            _seq = 0; // to do wait for taks then reset to 0
        }
    }
    return clients;
}



