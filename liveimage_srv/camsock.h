#ifndef CAMSOCK_H
#define CAMSOCK_H

#include <set>
#include "rawsock.h"
#include "pipefile.h"

class CamSock;
class CliSock;

class CamSock : public ConnSock
{
public:
    CamSock(ConnSock& ,
            const LiFrmHdr& h, const char* pextra);
    virtual ~CamSock();
    virtual void bind(CliSock* b, bool addremove);
    virtual int transfer(const std::vector<ConnSock*>& clis);
    virtual bool destroy(bool be=true);
    virtual void can_send(bool force=false);
private:
    bool      _validMagic(const LiFrmHdr* pms);
    bool      _validMagic(const LiFrmHdr* pms, const uint8_t* pmem);
    int       _deliverChunk(const uint8_t* vf, int len);
    size_t    _deliverFrame(int);

private:
    PipeFile*   _pf = nullptr;
    std::set<CliSock*>  _pclis;
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
    std::string _on_conhdr;
};

#endif // CAMSOCK_H
