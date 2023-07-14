#ifndef _LILITYPES_H_
#define _LILITYPES_H_

#include <assert.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <sys/time.h>


#define     INITIAL_LEN     32768
#define     STEP_LEN        4096


#define          PACK_ALIGN_1   __attribute__((packed, aligned(1)))


struct rect_t{
    int x;
    int y;
    int X;
    int Y;
};

struct dims_t{
    int x;
    int y;
};

class Frame
{
public:
    Frame(int cap=INITIAL_LEN):cap(INITIAL_LEN),len(0){
        buf = new uint8_t[cap];
        _ready = false;
        assert(buf);
    }
    ~Frame(){
        delete[] buf;
    }
    size_t ptr(const uint8_t** pb)const{
        *pb = buf;
        return len;
    }
    bool realloc(size_t nlen){
        if(nlen > cap){
            size_t newcap = ((nlen/STEP_LEN)+1)*STEP_LEN;
            uint8_t* pnewbuf = new uint8_t[newcap];
            if(pnewbuf){
                if(len){
                    ::memcpy(pnewbuf, buf, len);
                }
                delete[] buf;
                cap = newcap;
                buf = pnewbuf;
                return true;
            }
            return  false;
        }
        return true;
    }
    void ready(){_ready=true;};
    void append(const uint8_t* p, size_t nlen)
    {
        copy(p,len,nlen);
    }
    void copy(const uint8_t* p, size_t off, size_t nlen)
    {
        bool real=true;

        if(len+nlen > cap){
            real=realloc(nlen+len);
        }
        if(real){
            ::memcpy(buf+off, p, nlen);
            len = nlen+off;
        }else{
            len=0;
        }
    }
    void    set_len(size_t l){len=l;}
    size_t  length()const{return len;}
    size_t  capa()const{return cap;}
    size_t  room()const{return cap-len;}
    void    reset(){len=0;_ready = false;};
    uint8_t*     buffer(int off=0){
        return buf+off;
    }
    bool is_ready()const{return _ready && _wh.x;}
    dims_t  _wh = {0,0};

private:
    uint8_t* buf=nullptr;
    size_t  cap=0;
    size_t  len=0;
    bool    _ready;
};


enum EIMG_FMT{eNONE=-1, eFJPG=0, eNOTJPG, e422};


#define JPEG_MAGIC        0x12345678
#define MAX_PIX_MOVE      255
#define EVENTS_CUST       16
#define ONE_PAGE          4096
#define EVT_KEEP_ALIVE       0x80
#define CMD_RECORD        0x1
#define CMD_SAVLOC        0x2
#define EVT_JUST_MOTION   0x4
#define FLG_STAMP       0x40
#define EXTRA_SPACE     4096


#define EVT_KEEP_ALIVE  0x80
#define CMD_RECORD      0x1
#define CMD_SAVLOC      0x2
#define EVT_MOTION      (0x4|EVT_KEEP_ALIVE)
#define EVT_TLAPSE      (0x8|EVT_KEEP_ALIVE)
#define EVT_SIGNAL      (0x10|EVT_KEEP_ALIVE)
#define EVT_FORCE       (0x20|EVT_KEEP_ALIVE)




struct  event_t {
    uint8_t     predicate;
    uint16_t    movepix:12;
}PACK_ALIGN_1;

struct  LiFrmHdr{
    uint32_t    len;
    uint32_t    magic;
    uint32_t    mac;
    uint32_t    index;
    uint16_t    random;
    uint16_t    udppunch;
    uint16_t    wh[2];
    uint8_t     format:6;
    uint8_t     insync:2;
    event_t     event;
    uint8_t     challange[16];
    char        camname[16];
}PACK_ALIGN_1;

inline bool is_jpeg(const uint8_t* pb, int len){
    return len>10 && pb[0]==0xFF && pb[6]=='J' && pb[9]=='F';
}

struct imglayout_t{
    imglayout_t(){::memset(this,0,sizeof(*this));}
    const uint8_t *_camp = nullptr;
    size_t         _caml = 0;
    EIMG_FMT       _camf = e422;
    const uint8_t *_jpgp = nullptr;
    size_t         _jpgl = 0;
    EIMG_FMT       _jpgf = eFJPG;
    dims_t         _dims = {0,0};
    time_t         _now;
};

struct WebDblFrame
{
    LiFrmHdr*   _hdr      = nullptr;
    size_t      _cap      = 0;
    uint8_t*    _frame[2] = {nullptr,nullptr};
    size_t      _len[2]   = {0,0};

    void copy(const uint8_t* pb, size_t len, int index){
        if(len > _cap){
            len += EXTRA_SPACE;
            _frame[0] = new uint8_t[sizeof(LiFrmHdr) + len];
            _frame[1] = new uint8_t[sizeof(LiFrmHdr) + len];
            _cap = len;
        }
        _hdr = (LiFrmHdr*)_frame[index];
        _hdr->len = 0;
        ::memcpy(_frame[index]+sizeof(LiFrmHdr), pb, len);
        _len[index] = len;
    }
    WebDblFrame(){
        _frame[0] = new uint8_t[INITIAL_LEN + sizeof(LiFrmHdr)];
        _frame[1] = new uint8_t[INITIAL_LEN + sizeof(LiFrmHdr)];
        _cap = INITIAL_LEN;
        _hdr = (LiFrmHdr*)_frame[0];
    }
    ~WebDblFrame(){
        delete[] _frame[0];
        delete[] _frame[1];
    }
    const uint8_t* buff(int index){return _frame[index];}
    uint8_t* img(int index){return _frame[index]+sizeof(LiFrmHdr);}
    LiFrmHdr* hdr(int index){return (LiFrmHdr*)_frame[index];}
    size_t frm_len(int index){return _len[index] + sizeof(LiFrmHdr);}
};


inline int parse_url(const char* url, char* scheme, size_t
                     maxSchemeLen, char* host, size_t maxHostLen,
                     int* port, char* path, size_t maxPathLen) //Parse URL
{
    (void)maxPathLen;
    char* schemePtr = (char*) url;
    char* hostPtr = (char*) strstr(url, "://");
    if(hostPtr == NULL)
    {
        printf("Could not find host");
        return 0; //URL is invalid
    }

    if( maxSchemeLen < (size_t)(hostPtr - schemePtr + 1 )) //including NULL-terminating char
    {
        printf("Scheme str is too small (%zu >= %zu)", maxSchemeLen,
               hostPtr - schemePtr + 1);
        return 0;
    }
    memcpy(scheme, schemePtr, hostPtr - schemePtr);
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr+=3;

    size_t hostLen = 0;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL )
    {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf(portPtr, "%d", port) != 1)
        {
            printf("Could not find port");
            return 0;
        }
    }
    else
    {
        *port=80;
    }
    char* pathPtr = strchr(hostPtr, '/');
    if( hostLen == 0 )
    {
        hostLen = pathPtr - hostPtr;
    }

    if( maxHostLen < hostLen + 1 ) //including NULL-terminating char
    {
        printf("Host str is too small (%zu >= %zu)", maxHostLen, hostLen + 1);
        return 0;
    }
    memcpy(host, hostPtr, hostLen);
    host[hostLen] = '\0';

    size_t pathLen;
    char* fragmentPtr = strchr(hostPtr, '#');
    if(fragmentPtr != NULL)
    {
        pathLen = fragmentPtr - pathPtr;
    }
    else
    {
        if(pathPtr)
            pathLen = strlen(pathPtr);
        else
            pathLen=0;
    }

    if(pathPtr)
    {
        memcpy(path, pathPtr, pathLen);
        path[pathLen] = '\0';
    }
    else
    {
        path[0]=0;
    }

    return 1;
}


#endif
