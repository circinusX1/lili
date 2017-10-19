
#include <string.h>
#include <assert.h>
#include "videoframe.h"
#include "framequeue.h"
#include "main.h"

#define OUT_STREAM  std::cout

umutex       VideoFrame::_Mut;
VideoFrame   VideoFrame::_Frames[MAX_FRAMES];
int          VideoFrame::_Free=MAX_FRAMES;
static int SeqF = 0;

VideoFrame::VideoFrame(VideoFrame::ALLOCF b):_stack(false)
{
    _seq = ++SeqF;

    if(b==POOL)
        return; /**/
    _buff = new uint8_t[MAX_BUFF+1];
    _cap = MAX_BUFF;
    _stack = true;
}

VideoFrame::~VideoFrame()
{
    if(_stack){
        delete[] _buff;
        _buff = nullptr;
    }
}

/**
 * @brief VideoFrame::append
 * have to send in a chunk precise no of entire frames
 * with HIK delimiter, because when we drop frame we cannot
 * drop half info
 * @param buff
 * @param bytes
 * @param pfq
 * @return
 */
int  VideoFrame::append(const uint8_t* buff, int bytes)
{
    //GLOGD(__FUNCTION__<<_frmlen<<", seq:" << _seq <<", accum:"<<_accum<<", " << _frmlen);
    assert(_frmlen);
    int room = _frmlen - _accum;
    if((int)bytes > room){
        bytes = room;
    }
    ::memcpy(_buff+_accum,buff,bytes);
    _accum += bytes;
    return bytes;
}

void    VideoFrame::setFrmLen(int fl, bool temporarily)
{
    _tempframe = temporarily;
    if(_cap < fl)
    {
        delete _buff;
        _buff = new uint8_t [fl+sizeof(size_t)];
        _cap=fl+sizeof(size_t);
    }
    _frmlen=fl;
    //GLOGD(__FUNCTION__<<_frmlen<<", seq:" << _seq <<", accum:"<<_accum<<", " << _frmlen);
}

void VideoFrame::print(int c)const
{
    GLOGI (_seq << " Frm LEN:" <<
                 _frmlen << " Accum " <<
                 _accum << " bytes in "
                 << c  );
}

void VideoFrame::reset()
{
    _accum=0;
    _tempframe=false;
    _busy=false;
    _frmlen=0;
    _seq = ++SeqF;
    //std::cout << "\n NEW FRAME  "<< _seq << "\n";
    //GLOGD(__FUNCTION__<<_frmlen<<", seq:" << _seq <<", accum:"<<_accum<<", " << _frmlen);
}

void VideoFrame::save(int seq)
{
    char fn[64];
    sprintf(fn,"test%d.jpg",seq);
    FILE* pf = ::fopen(fn,"wb");
    if(pf)
    {
        ::fwrite(_buff,1, _accum, pf);
        ::fclose(pf);
    }
}

void     VideoFrame::resetCache()
{
    AutoLock guard(&_Mut);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        _Frames[i].reset();
    }
    VideoFrame::_Free=MAX_FRAMES;
}

void* VideoFrame::operator new(size_t osz)throw()
{
    AutoLock guard(&_Mut);
    static bool Exhaust = false;
    UNUS(osz);
    for(size_t i=0; i<MAX_FRAMES; ++i)
    {
        if(_Frames[i].busy()==false)
        {
            _Frames[i]._now = SECS();
            _Frames[i].reset();
            _Frames[i].busy(true);
            _Frames[i]._stack=false;
            --VideoFrame::_Free;
            Exhaust=false;
            return (void*)&_Frames[i];
        }
    }
    if(Exhaust==false)
    {
        Exhaust=true;
        OUT_STREAM << "dropping frame. Pusher failed to eat\n";
    }
    return nullptr; //warn supress
}

void   VideoFrame::operator delete(void* pv, std::size_t sz)
{
    AutoLock guard(&_Mut);
    UNUS(sz);
    VideoFrame* pvf = reinterpret_cast<VideoFrame*>(pv);
    pvf->reset();

    ++VideoFrame::_Free;
}

int VideoFrame::cacheRoom()
{
    AutoLock guard(&_Mut);
    return VideoFrame::_Free;
}

