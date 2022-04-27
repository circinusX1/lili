#ifndef LOCALCAM_H
#define LOCALCAM_H

#include "os.h"
#include "sock.h"
#include "v4ldevice.h"
#include "acamera.h"

class localcam : public acamera
{
public:
    localcam(const dims_t& wh,const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~localcam();
    virtual size_t get_frame(const uint8_t** pb, EIMG_FMT& fmt);
    virtual bool spin();
    virtual bool init(const dims_t&);

private:
    std::string     _device;
    v4ldevice*      _dev=nullptr;
    dims_t          _img_size;
    EIMG_FMT        _fmt;
};

#endif // LOCALCAM_H
