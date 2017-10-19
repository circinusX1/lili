#ifndef FRAMEQUEUE_H
#define FRAMEQUEUE_H

#include "os.h"
#include <deque>



typedef enum{
    CACHE_EXHAUSTED = 1,
    FRMTOOSMALL = 2,
}NO_CACHEENUM;

class VClient;
class CmdQue;
class FrameQueue
{
public:
    FrameQueue(){};
    ~FrameQueue();

    VideoFrame* deque();
    void enque(VideoFrame*);
private:
    std::deque<VideoFrame*>     _frames;
    umutex                      _m;
};

#endif // FRAMEQUEUE_H
