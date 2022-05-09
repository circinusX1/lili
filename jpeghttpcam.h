#ifndef JPEGHTTPCAM_H
#define JPEGHTTPCAM_H

#include "os.h"
#include "lilitypes.h"
#include "acamera.h"

class jpeghttpcam : public acamera, public osthread
{
public:
    jpeghttpcam(const dims_t& wh, const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~jpeghttpcam();
    virtual size_t get_frame(imglayout_t& i);
    virtual bool spin();
    virtual bool init(const dims_t&);
    virtual void thread_main();

private:


private:
    bool            _moved = true;
    int             _ifrm = 0;
    dims_t          _imgsz = {0,0};
    EIMG_FMT        _format;
    mutexx          _mut;
    int             _msleep = 1000;
};

#endif // JPEGHTTPCAM_H
