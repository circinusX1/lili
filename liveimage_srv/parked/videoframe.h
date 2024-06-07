#ifndef VIDEOFRAME_H
#define VIDEOFRAME_H

#include "os.h"
#include <iostream>
#include <stdint.h>
#include <vector>

#define MAX_BUFF    (1024*32)
#define MAX_FRAMES  32       //min 1 sec
#define ADD_ON_BUFF (1024 * 4)

class FrameQueue;
class VideoFrame
{
public:
    typedef enum{STACK=0,POOL}ALLOCF;
    explicit VideoFrame(VideoFrame::ALLOCF b);
    virtual ~VideoFrame();
    static int cacheRoom();
    void     reset();
    int      seq()const{return _seq;}
    void     print(int)const;
    bool     busy()const {return _busy;};
    void     busy(bool b){_busy=b;_accum=0;};
    const uint8_t* buffer()const{return _buff;}
    uint8_t* wbuffer(){return _buff;}
    void     bufend(size_t be){_accum=be;}
    int      append(const uint8_t* buff, int bytes);
    int      length()const{return _accum;}
    int      size()const{return MAX_BUFF;}
    bool     hasRoom(int r){return MAX_BUFF-(_accum)>r;}
    void     save(int);
    int      room()const{return (int)(MAX_BUFF-(_accum));}
    int      frmlen()const{return _frmlen;}
    void    setFrmLen(int  fl, bool temporarily=false);
    bool    is_temp()const{return _tempframe;}
private:
    explicit VideoFrame():_accum(0),_cap(MAX_BUFF)
    {
        _buff = new uint8_t[MAX_BUFF+1];
        if(_buff==0)
        {
            printf("cannot allocate heap \r\n");
            exit(0);
        }
    }
    bool _resync(size_t& offset);

public:
    static void*    operator new(size_t sz)throw();
    static void     operator delete(void* pv, std::size_t sz);
    static void     resetCache();
public:
    time_t      _now;
protected:
    int         _accum;
    int         _cap;
    bool        _busy;
    int         _frames;
    int         _frmlen;
    uint8_t*    _buff;
    int         _seq;
    bool        _tempframe =false;;
    bool        _stack=false;
private:

    static umutex       _Mut;
    static VideoFrame   _Frames[MAX_FRAMES];
    static uint8_t*     _vchunks;
    static size_t       _vaccum;
    static int          _Free;
};

#define FROM_POOL true

#endif // VIDEOFRAME_H
