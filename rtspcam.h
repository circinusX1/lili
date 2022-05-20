
#ifndef RTSPCAM_H
#define RTSPCAM_H

#include <string>
#include <map>
#include "os.h"
#include "webcast.h"
#include "acamera.h"
#include "pipefile.h"


class rtpudpcs;
class rtspcam : public osthread, public acamera
{
public:
    rtspcam(const dims_t& wh,const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~rtspcam();
    virtual void  thread_main();
    virtual size_t get_frame(imglayout_t& i);
    virtual bool spin();
    virtual bool init(const dims_t&);

private:
#ifdef WITH_AVLIB_RTSP
    void    _avlib_rtsp();
#endif

private:
    std::string             _url,_user,_uri;
    long                    _seq  = 0;
    unsigned long           _curl_auth = 0;
    mutexx                  _mut;
    bool                    _ontcp = false;
    pipiefile*              _pipa = nullptr;  //("/tmp/movie.mov");
};

#endif // RTSPCAM_H

