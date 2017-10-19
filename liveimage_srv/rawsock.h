///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef RAWSOCK_H
#define RAWSOCK_H

#include <string>
#include <map>
#include <vector>
#include "sock.h"
#include "logger.h"

#define MAX_BUFF  128000

extern std::string empty_string;
#define          PACK_ALIGN_1   __attribute__((packed,aligned(1)))

struct  LiFrmHdr{
    uint32_t    len;
    uint32_t    magic;
    uint32_t    mac;
    uint32_t    index;
    uint16_t    random;
    uint16_t    udppunch;
    uint16_t    wh[2];
    uint8_t     format:3;
    uint8_t     insync:1;
    uint8_t     record:2;
    uint8_t     event;          // lapse 1 movepix > 1
    uint8_t     challange[16];
    char        camname[16];
}PACK_ALIGN_1;

class Sheller;
class RawSock : public tcp_cli_sock
{
public:
    typedef enum _STYPE{
        NONE = 0,
        SRV,
        CAM,
        CLIRTSP,
        CLIJPEG,
        CLIENT,
        BROKEN
    }STYPE;

    struct Data
    {
        uint8_t   * buff = nullptr;
        size_t   rec_off  = 0;
        size_t   prc_off  = 0;
        size_t   vfl    = 0;
        size_t   cap    = 0;
        size_t   inbytes = 0;
        size_t   outbytes = 0;
        bool     inuse  = false;
        void     reset(){prc_off=0;rec_off=0;vfl=0;}
        size_t   room(){return cap-rec_off;}
        uint8_t* rec_ptr(){return buff + rec_off;}
        uint8_t* prc_ptr(){return buff + prc_off;}
        void     advance_prc(size_t t){prc_off+=t;}
        size_t   left(){return rec_off-prc_off;}
        void print(const char* p)
        {
            GLOGD(p<<" rd=" << prc_off << " wr=" << rec_off <<
                  " inbuff="<<left() << " to-Fill=" << int(vfl-left()) << " vfl="<<vfl << " room="<<room()<<
                  " diff " << (int)inbytes-outbytes);
        }
    };

    RawSock(RawSock& r,
             const LiFrmHdr& h,STYPE t);
    RawSock(){_data.buff=(nullptr);_data.cap=0;};
    virtual ~RawSock();
    virtual bool isopen();
    virtual bool destroy(bool be=true);
    virtual int  recdata(uint8_t* buff,size_t rec_off);
    virtual int  snd(const uint8_t* b,size_t room,uint32_t extra,const char* oth);
    virtual int    transfer(const std::vector<RawSock*>& clis);
    virtual int recdata();
    virtual void   can_send(bool force=false);
    const std::string name()const{return _name;}
    const LiFrmHdr*  header()const{return &_header;}
    STYPE type()const {return _t;}
    time_t  last_time()const{return _nowt;}
    void    set_letal(){_letal=true;}
    void    bind(Sheller* p){_tm=p;}
protected:
    time_t      _nowt = 0;
    std::string _mac;
    std::string _name;
    LiFrmHdr    _header;
    bool        _letal=false;
    STYPE       _t;          // avoid dynamic casts
    Data        _data;
    Sheller*    _tm;
};



#endif // RAWSOCK_H
