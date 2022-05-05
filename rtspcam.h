
#ifndef RTSPCAM_H
#define RTSPCAM_H

#include <string>
#include <map>
#include <curl/curl.h>
#include "os.h"
#include "webcast.h"
#include "acamera.h"
#include "pipefile.h"


#define VF_LEN      8912
#define VF_LEN_ADD  1024

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


private:
    std::string             _url,_transport,_user,_uri;
    long                    _seq  = 0;
    unsigned long           _curl_auth = CURLAUTH_NONE;
    Frame                   _frames[2];
    int                     _flip = 0;
    mutexx                  _mut;
    bool                    _ontcp = false;

};

#endif // RTSPCAM_H

