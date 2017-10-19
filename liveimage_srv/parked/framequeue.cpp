
#include <assert.h>
#include "main.h"
#include "framequeue.h"


#define OUT_STREAM  nullout

/**
 * @brief FrameQueue::~FrameQueue
 */
FrameQueue::~FrameQueue()
{
    AutoLock guard(&_m);
    while(_frames.size())
    {
        VideoFrame* pv = _frames.front();
        _frames.pop_front();
        delete pv;
    }
}

/**
 * @brief FrameQueue::enque
 * @param vf
 */
void FrameQueue::enque(VideoFrame* vf)
{
    AutoLock guard(&_m);
    if(vf->length())
        _frames.push_back(vf);
}

/**
 * @brief FrameQueue::deque
 * @return
 */
VideoFrame* FrameQueue::deque()
{
    AutoLock guard(&_m);
    if(_frames.size())
    {
        //std::cout << "q=" <<_frames.size() << "\n";
        VideoFrame* pv = _frames.front();
        _frames.pop_front();
        return pv;
    }
    return nullptr;
}
