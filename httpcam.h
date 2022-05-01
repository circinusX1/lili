#ifndef HTTPCAM_H
#define HTTPCAM_H

#include "os.h"
#include "lilitypes.h"
#include "acamera.h"

class httpcam : public acamera, public osthread
{
public:
    httpcam(const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~httpcam();
    virtual size_t get_frame(const uint8_t** pb, EIMG_FMT& fmt, event_t& event);
    virtual bool spin(event_t& event);
    virtual bool init(const dims_t&);
    virtual void thread_main();

private:
    void    to_gray();

private:
    bool            _moved = true;
    Frame           _frame[2];
    int             _ifrm = 0;
    dims_t          _imgsz = {0,0};
    std::string     _motionurl;
    std::string     _motiontoken;
    EIMG_FMT        _format;
    mutexx          _mut;
    int             _msleep = 1000;
};

#endif // HTTPCAM_H
