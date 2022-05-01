#ifndef _LILITYPES_H_
#define _LILITYPES_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define     INITIAL_LEN     16386
#define     STEP_LEN        4096


#define          PACK_ALIGN_1   __attribute__((packed, aligned(1)))



class Frame
{
public:
    Frame(int cap=INITIAL_LEN):cap(INITIAL_LEN),len(0){
        buf = new uint8_t[cap];
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
                return true;
            }
            return  false;
        }
        return true;
    }
    void copy(const uint8_t* p, size_t off, size_t nlen)
    {
        bool real=true;
        if(nlen > cap){
            real=realloc(nlen);
        }
        if(real){
            ::memcpy(buf+off, p, nlen);
            len = nlen;
        }else{
            len=0;
        }
    }
    void    set_len(size_t l){len=l;}
    size_t  length()const{return len;}
    size_t  capa()const{return cap;}
    void    reset(){len=0;};
    uint8_t*     buffer(int off=0){
        return buf+off;
    }
private:
    uint8_t* buf=nullptr;
    size_t  cap=0;
    size_t  len=0;
};


enum EIMG_FMT{eNONE=-1, eFJPG=0, eNOTJPG, e422};


#define JPEG_MAGIC        0x12345678
#define MAX_PIX_MOVE      255
#define EVENTS_CUST       16

#define EVT_KKEP_ALIVE  0x80
#define CMD_RECORD      0x1
#define CMD_SAVLOC      0x2
#define EVT_JUST_MOTION      0x4
#define EVT_MOTION      0x4|0x80
#define EVT_TLAPSE      0x8|0x80
#define EVT_SIGNAL      0x10|0x80
#define EVT_FORCE       0x20|0x80
#define FLG_STAMP       0x40

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

struct imglayout_t{
    imglayout_t(){::memset(this,0,sizeof(*this));}
    const uint8_t *_camp = nullptr;
    size_t         _caml = 0;
    EIMG_FMT       _camf = e422;
    const uint8_t *_jpgp = nullptr;
    size_t         _jpgl = 0;
    EIMG_FMT       _jpgf = eFJPG;
    dims_t         _dims = {0,0};
};


#endif
