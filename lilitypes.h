#ifndef _LILITYPES_H_
#define _LILITYPES_H_

#include <assert.h>
#include <stdint.h>
#include <string.h>

#define     INITIAL_LEN     4096
#define     STEP_LEN        1024


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
    void copy(const uint8_t* p, size_t nlen)
    {
        if(nlen > cap){
            delete[] buf;
            cap = ((nlen/STEP_LEN)+1)*STEP_LEN;
            buf = new uint8_t[cap];
            assert(buf);
        }
        ::memcpy(buf, p, nlen);
        len = nlen;
    }
    void    set_len(size_t l){len=l;}
    size_t  length()const{return len;}
    size_t  capa()const{return cap;}
    void    reset(){len=0;};
    uint8_t*     buffer(int off=0){return buf+off;}
private:
    uint8_t* buf=nullptr;
    size_t  cap=0;
    size_t  len=0;
};


enum EIMG_FMT{eFJPG, eFMPG, e422};


#define JPEG_MAGIC        0x12345678
#define MAX_PIX_MOVE      255
#define EVENTS_CUST       16

#define CMD_RECORD      0x1
#define CMD_SAVLOC      0x2
#define EVT_MOTION      0x4
#define EVT_TLAPSE      0x8
#define EVT_SIGNAL      0x10
#define EVT_FORCE       0x20

struct  event_t {
    uint8_t     predicate;
    uint16_t    movepix:12;
}PACK_ALIGN_1;


#define         PTRU_TOUT  30


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


#endif
