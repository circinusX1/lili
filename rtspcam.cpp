#ifdef WITH_RTSP
#include <stdio.h>
#include <string.h>
#include "rtspcam.h"
#include "cbconf.h"
#include "rtpudpcs.h"


extern bool __alive;
static rtspcam* _pthis;


#define OUT_STREAM  nullout

rtspcam::rtspcam(const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(name,loc,n)
{
    _url = loc;
    _transport = "RTP/AVP;unicast;client_port=";
    std::string auth = n["login"].value(0);
    if(auth=="BASIC")
        _curl_auth = CURLAUTH_BASIC;
    else if(auth=="DIGEST")
        _curl_auth = CURLAUTH_DIGEST;
    if(auth=="NEGOCIATE")
        _curl_auth = CURLAUTH_NEGOTIATE;
    _user      = n["login"].value(1);
    _pass      = n["login"].value(2);


    _pthis = this;
}

rtspcam::~rtspcam()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
    delete _pudpsink;
}


void  rtspcam::thread_main()
{
    event_t ev;
    _pcurl_lib = ::dlopen(CFG["image"]["lib_curl"].value().c_str(), RTLD_NOW);
    if(_pcurl_lib == nullptr)
    {
        char* error = dlerror();
        if (error != NULL) {
            fprintf(stderr, "%s\n", error);
            return;
        }
    }
    SO_SYM(_pcurl_lib, curl_global_init);
    SO_SYM(_pcurl_lib, curl_easy_init);
    SO_SYM(_pcurl_lib, curl_easy_setopt);
    SO_SYM(_pcurl_lib, curl_easy_perform);
    SO_SYM(_pcurl_lib, curl_easy_cleanup);
    SO_SYM(_pcurl_lib, curl_global_cleanup);
    SO_SYM(_pcurl_lib, curl_easy_getinfo);
    SO_SYM(_pcurl_lib, curl_slist_append);
    SO_SYM(_pcurl_lib, curl_slist_free_all);


    _get_sdp_filename();
    CURLcode res = _pcurl_global_init(CURL_GLOBAL_ALL);
    if(res == CURLE_OK)
    {
        _curl = _pcurl_easy_init();
        if(_curl) {
            int port;
            for(port=6000;port<6100;port++)
            {
                _pudpsink = new rtpudpcs(port);
                ::usleep(0xFFFF);
                if(_pudpsink->ok())
                {
                    break;
                }
                ++port;
                delete _pudpsink;
            }
            if(_pudpsink->ok()==false)
            {
                return;
            }
            _transport+=std::to_string(port);
            _transport+="-";
            _transport+=std::to_string(port+1);
            my_curl_easy_setopt(_curl, CURLOPT_VERBOSE, 1L);
            my_curl_easy_setopt(_curl, CURLOPT_NOPROGRESS, 1L);
            my_curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, rtspcam::_write_callback);
            my_curl_easy_setopt(_curl, CURLOPT_READFUNCTION, rtspcam::_read_callback);
            my_curl_easy_setopt(_curl, CURLOPT_HEADERFUNCTION, rtspcam::_hdr_callback);
            my_curl_easy_setopt(_curl, CURLOPT_CONNECTTIMEOUT, 10);
            my_curl_easy_setopt(_curl, CURLOPT_ACCEPTTIMEOUT_MS, 8000);
            my_curl_easy_setopt(_curl, CURLOPT_WRITEDATA, NULL);
            my_curl_easy_setopt(_curl, CURLOPT_URL, _url.c_str());
            _pcurl_easy_getinfo(_curl, CURLINFO_RTSP_CSEQ_RECV, &_seq);
            my_curl_easy_setopt(_curl, CURLOPT_RTSP_CLIENT_CSEQ, _seq);

            _rtsp_options();
            _rtsp_describe();
            _get_media_control_attribute();
            char uri[400];
            snprintf(uri, sizeof(uri), "%s/%s", _url.c_str(), _control);
            _rtsp_setup(uri);
            this->_rtsp_play();
            while(__alive && !this->is_stopped())
            {
                int bytes = _pudpsink->spin(ev);
                if(bytes)
                {
                    AutoLock    a(&_mut);
                    Frame& frame = _frames[_flip];
                    frame.copy(_pudpsink->frame(), bytes);
                }
                ::usleep(0x1FFF);
            }
            _rtsp_teardown();
            _pcurl_easy_cleanup(_curl);
            _pcurl_global_cleanup();
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t rtspcam::get_frame(const uint8_t** pb, EIMG_FMT& fmt, event_t&)
{
    size_t          leng = 0;
    AutoTryLock    a(&_mut);

    Frame& frame = _frames[_flip];
    if(frame.length())
    {
        leng = frame.ptr(pb);
        fmt = eFMPG;
        _flip = !_flip;
    }
    fmt = eFMPG;
    return leng;
}


size_t rtspcam::_hdr_callback(char *ptr, size_t, size_t nmemb, void *)
{
    TRACE() << __FUNCTION__ << "HDR: [" << ptr << "]\r\n";
    _pthis->_describe.append(ptr);
    return nmemb;
}

size_t rtspcam::_write_callback(char *, size_t size, size_t nmemb, void *)
{
    TRACE() << __FUNCTION__ << size << " bytes\r\n";
    return nmemb;
}
size_t rtspcam::_read_callback(char *, size_t size, size_t nmemb, void *)
{
    TRACE() << __FUNCTION__ << size << " bytes\r\n";
    return nmemb;
}

void rtspcam::_rtsp_options()
{
    printf("\nRTSP: OPTIONS %s\n", _url.c_str());
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_STREAM_URI, _url.c_str());
    if(!_user.empty())
    {
        char up[128];
        ::snprintf(up,127,"%s:%s",_user.c_str(),_pass.c_str());
        my_curl_easy_setopt(_curl, CURLOPT_USERPWD, up);
        my_curl_easy_setopt(_curl, CURLOPT_HTTPAUTH, (long)_curl_auth);
    }
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_OPTIONS);
    my_curl_easy_perform(_curl);
}

void rtspcam::_rtsp_describe()
{
    FILE *sdp_fp = fopen(_sdp_filename, "wb");
    printf("\nRTSP: DESCRIBE %s\n", _url.c_str());
    if(!sdp_fp) {
        fprintf(stderr, "Could not open '%s' for writing\n", _sdp_filename);
        sdp_fp = stdout;
    }
    else {
        printf("Writing SDP to '%s'\n", _sdp_filename);
    }
    _describe.clear();
    my_curl_easy_setopt(_curl, CURLOPT_WRITEDATA, sdp_fp);
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_DESCRIBE);
    my_curl_easy_perform(_curl);
    my_curl_easy_setopt(_curl, CURLOPT_WRITEDATA, stdout);
    if(sdp_fp != stdout) {
        fwrite(_describe.c_str(), 1, _describe.length(), sdp_fp);
        fclose(sdp_fp);
    }
    TRACE() << "DECRIBED \n[" << _describe << "]\n";
}

void rtspcam::_rtsp_setup(const char *uri)
{
    printf("\n----------------------------\nRTSP: SETUP %s\n", _url.c_str());
    printf("      TRANSPORT %s\n", _transport.c_str());

    my_curl_easy_setopt(_curl, CURLOPT_RTSP_STREAM_URI, uri);
    my_curl_easy_setopt(_curl, CURLOPT_TIMEOUT_MS, 4000);
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_TRANSPORT, _transport.c_str());
    my_curl_easy_setopt(_curl, CURLOPT_USERPWD, nullptr);
    my_curl_easy_setopt(_curl, CURLOPT_HTTPAUTH, (long)CURLAUTH_NONE);
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_SETUP);
    my_curl_easy_perform(_curl);
    printf("\n------------------------------------\n");
}

void rtspcam::_rtsp_play()
{
    const char *range = "0.000-";

    printf("\nRTSP: PLAY %s\n", _url.c_str());
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_STREAM_URI, _url.c_str());
    my_curl_easy_setopt(_curl, CURLOPT_RANGE, range);
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_PLAY);
    my_curl_easy_perform(_curl);

    /* switch off using range again */
    my_curl_easy_setopt(_curl, CURLOPT_RANGE, NULL);
}

void rtspcam::_rtsp_teardown()
{
    printf("\nRTSP: TEARDOWN %s\n", _url.c_str());
    my_curl_easy_setopt(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_TEARDOWN);
    my_curl_easy_perform(_curl);
}

void rtspcam::_get_sdp_filename()
{
    const char *s = strrchr(_url.c_str(), '/');
    strcpy(_sdp_filename, "video.sdp");
    if(s) {
        s++;
        if(s[0] != '\0') {
            snprintf(_sdp_filename, sizeof(_sdp_filename), "%s.sdp", s);
        }
    }
}


void rtspcam::_get_media_control_attribute()
{
    char s[256]={0};

    FILE *sdp_fp = fopen(_sdp_filename, "rb");
    _control[0] = '\0';
    if(sdp_fp) {
        while(fgets(s, sizeof(s)-2, sdp_fp)) {
            sscanf(s, " a = control: %32s", _control);
        }
        printf("\r\nSDP: [%s]\r\n",s);
        fclose(sdp_fp);
    }
}

bool rtspcam::spin(event_t&)
{
    return true;
}



bool rtspcam::init(const dims_t&)
{
    return start_thread()==0;
}

#endif
