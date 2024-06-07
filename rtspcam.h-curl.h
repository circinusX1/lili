#ifdef WITH_RTSP

#ifndef RTSPCAM_H
#define RTSPCAM_H

#include <string>
#include <map>
#include <curl/curl.h>
#include "os.h"
#include "webcast.h"
#include "acamera.h"

#define VF_LEN      8912
#define VF_LEN_ADD  1024

typedef CURL*    (*p_curl_easy_init)(void);
typedef CURLcode (*p_curl_easy_setopt)(CURL *curl, CURLoption option, ...);
typedef CURLcode (*p_curl_easy_perform)(CURL *curl);
typedef void     (*p_curl_easy_cleanup)(CURL *curl);
typedef CURLcode (*p_curl_global_init)(long flags);
typedef void     (*p_curl_global_cleanup)(void);
typedef CURLcode (*p_curl_easy_getinfo)(CURL *curl, CURLINFO info, ...);
typedef struct curl_slist* (*p_curl_slist_append)(struct curl_slist *,const char *);
typedef void (*p_curl_slist_free_all)(struct curl_slist *);


class rtpudpcs;
class rtspcam : public osthread, public acamera
{
public:
    rtspcam(const dims_t& wh,const std::string& name, const std::string& loc, const Cbdler::Node& n);
    virtual ~rtspcam();
    virtual void  thread_main();
    virtual size_t get_frame(const uint8_t** pb, EIMG_FMT& fmt);
    virtual bool spin();
    virtual bool init(const dims_t&);

private:
    void _perform();
    void _get_sdp_filename();
    void _rtsp_describe();
    void _get_media_control_attribute();
    void _rtsp_setup();
    void _rtsp_play();
    void _rtsp_teardown();

    static size_t _read_callback(char *ptr, size_t size, size_t nmemb, void *ud);
    static size_t _write_callback(char *ptr, size_t size, size_t nmemb, void *ud);
    static size_t _hdr_callback(char *ptr, size_t size, size_t nmemb, void *ud);
    static size_t _interleave_callback(void *ptr, size_t size, size_t nmemb, void *up);

private:
    void*                   _pcurl_lib = nullptr;
    p_curl_global_init      _pcurl_global_init = nullptr;
    p_curl_easy_init        _pcurl_easy_init = nullptr;
    p_curl_easy_setopt      _pcurl_easy_setopt = nullptr;
    p_curl_easy_perform     _pcurl_easy_perform = nullptr;
    p_curl_easy_cleanup     _pcurl_easy_cleanup = nullptr;
    p_curl_global_cleanup   _pcurl_global_cleanup = nullptr;
    p_curl_easy_getinfo     _pcurl_easy_getinfo = nullptr;
    p_curl_slist_append     _pcurl_slist_append = nullptr;
    p_curl_slist_free_all   _pcurl_slist_free_all = nullptr;

    std::string             _name;
    std::string             _url;
    std::string             _user;
    std::string             _pass;
    std::string             _transport;
    std::string             _describe;
    std::string             _motion;
    char                    _sdp_filename[256];
    char                    _control[256];
    CURL*                   _curl = nullptr;
    rtpudpcs*               _pudpsink = nullptr;
    long                    _seq  = 0;
    unsigned long           _curl_auth = CURLAUTH_NONE;
    uint8_t*                _fr_buff[2] = {nullptr, nullptr};
    Frame                   _frames[2];
    int                     _flip = 0;
    mutexx                  _mut;
    bool                    _ontcp = false;
    rtspcam*                _pthis = nullptr;
    size_t                  _gotbytes = 0;
};

#define CU_SET(A, B, C)                               \
  do {                                                             \
    CURLcode res = this->_pcurl_easy_setopt((A), (B), (C));      \
    if(res != CURLE_OK)                                            \
      fprintf(stderr, "curl_easy_setopt(%s, %s, %s) failed: %d\n", \
              #A, #B, #C, res);                                    \
  } while(0)

#define my_curl_easy_perform(A)                                         \
  do {                                                                  \
    CURLcode res = this->_pcurl_easy_perform(A);                      \
    if(res != CURLE_OK)                                                 \
      fprintf(stderr, "curl_easy_perform(%s) failed: %d\n", #A, res);   \
  } while(0)


#endif // RTSPCAM_H

#else// WITH_RTSP

#warning "RTSP EXPERIMENTAL FEATURE IS DISABLED Use -DWITH_RTSP"

#endif
