#ifdef WITH_RTSP
#include <stdio.h>
#include <string.h>
#include "rtspcam.h"
#include "cbconf.h"
#include "rtpudpcs.h"


extern bool __alive;

#define OUT_STREAM  nullout

rtspcam::rtspcam(const dims_t& wh,const std::string& name,
                 const std::string& loc,
                 const Cbdler::Node& n):acamera(wh,name,loc,n)
{
    _url = loc;
    const std::string& proto = n["rtsp"].value();
    if(proto=="TCP"){
        _transport = "RTP/AVP/TCP;unicast;interleaved=0-1";
        _ontcp = true;
    }
    else
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

}

rtspcam::~rtspcam()
{
    this->signal_to_stop();
    ::usleep(0xFFFF);
    this->stop_thread();
    delete _pudpsink;
}



void  rtspcam::_perform()
{
    this->_pcurl_easy_perform(_curl);
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
    while(!this->is_stopped() && __alive)
    {
        CURLcode res = _pcurl_global_init(CURL_GLOBAL_ALL);
        if(res == CURLE_OK)
        {
            _curl = _pcurl_easy_init();
            if(_curl) {

                if(!_ontcp)
                {
                    int port;
                    _transport+=std::to_string(port);
                    _transport+="-";
                    _transport+=std::to_string(port+1);
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
                }

                CU_SET(_curl, CURLOPT_FOLLOWLOCATION, 1L);
                CU_SET(_curl, CURLOPT_VERBOSE, 1L);
                CU_SET(_curl, CURLOPT_NOPROGRESS, 1L);
                CU_SET(_curl, CURLOPT_URL, _url.c_str());
                CU_SET(_curl, CURLOPT_CONNECTTIMEOUT, 10);
                CU_SET(_curl, CURLOPT_ACCEPTTIMEOUT_MS, 8000);
               // CU_SET(_curl, CURLOPT_BUFFERSIZE,MAX_BUFF);

                CU_SET(_curl, CURLOPT_HEADERFUNCTION, rtspcam::_hdr_callback);
                CU_SET(_curl, CURLOPT_HEADERDATA, (void*)this);


                CU_SET(_curl, CURLOPT_RTSP_STREAM_URI,_url.c_str());
                if(!_user.empty())
                {
                    char up[128];
                    ::snprintf(up,127,"%s:%s",_user.c_str(),_pass.c_str());
                    CU_SET(_curl, CURLOPT_USERPWD, up);
                    CU_SET(_curl, CURLOPT_HTTPAUTH, (long)_curl_auth);
                }
                CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_OPTIONS);
                _perform();
                ///////////////////////////////////////////////////////////////////////////////////
                _rtsp_describe();
                _get_media_control_attribute();
                _rtsp_setup();
                _rtsp_play();
                int bytes = 128;
                while(__alive && !this->is_stopped() && (bytes/=2) > 0)
                {
                    if(_pudpsink)
                    {
                        int bytes = _pudpsink->spin();
                        if(bytes)
                        {
                            AutoLock    a(&_mut);
                            Frame& frame = _frames[_flip];
                            frame.copy(_pudpsink->frame(), 0, bytes);
                        }
                    }
                    else
                    {
                        AutoLock    a(&_mut);
                        bytes = _gotbytes;
                    }
                    ::usleep(0x1FFF);
                }
                _rtsp_teardown();
                _pcurl_easy_cleanup(_curl);
                _pcurl_global_cleanup();
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
size_t rtspcam::get_frame(const uint8_t** pb, EIMG_FMT& fmt)
{
    size_t          leng = 0;
    AutoTryLock    a(&_mut);

    if(a.locked())
    {
        Frame& frame = _frames[_flip];
        if(frame.length())
        {
            leng = frame.ptr(pb);
            fmt = eNOTJPG;
            _flip = !_flip;
        }
        fmt = eNOTJPG;
    }
    return leng;
}


size_t rtspcam::_interleave_callback(void *ptr,
                                     size_t size,
                                     size_t nmemb,
                                     void *up)
{
    rtspcam* pthis = static_cast<rtspcam*>(up);
    if(__alive==false){return 0;}
    if(nmemb)
    {
        AutoLock    a(&pthis->_mut);

        pthis-> _gotbytes = size * nmemb;
        Frame& frame = pthis->_frames[pthis->_flip];
        frame.copy((const uint8_t*)ptr, 0, nmemb);
        return nmemb;
    }
    OUT_STREAM << __FUNCTION__<< " streaming ended" << "\n";
    return 0;
}


size_t rtspcam::_hdr_callback(char *ptr, size_t size, size_t nmemb, void *up)
{
    TRACE() << __FUNCTION__ << "HDR: [" << ptr << "]\r\n";
    rtspcam* pthis = reinterpret_cast<rtspcam*>(up);
    pthis->_describe.append(ptr);
    return nmemb * size;
}

size_t rtspcam::_write_callback(char *, size_t size, size_t nmemb, void * up)
{
    TRACE() << __FUNCTION__ << size << " bytes\r\n";
    rtspcam* pthis = reinterpret_cast<rtspcam*>(up);
    if(pthis->_ontcp)
    {

        CURL* pcurl = pthis->_curl;

        if(__alive==false){return 0;}
        if(nmemb)
        {
            ::usleep(1000);
            if(pthis->_ontcp)
            {
                pthis->_pcurl_easy_setopt(pcurl, CURLOPT_INTERLEAVEFUNCTION, rtspcam::_interleave_callback);
                pthis->_pcurl_easy_setopt(pcurl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
                pthis->_pcurl_easy_perform(pcurl);
            }
        }
    }
    return size * nmemb;
}
size_t rtspcam::_read_callback(char *, size_t size, size_t nmemb, void *up)
{
    rtspcam* pthis = reinterpret_cast<rtspcam*>(up);
    TRACE() << __FUNCTION__ << size << " bytes\r\n";
    return nmemb * size;
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
    CU_SET(_curl, CURLOPT_WRITEDATA, sdp_fp);
    CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_DESCRIBE);
    _perform();
    CU_SET(_curl, CURLOPT_WRITEDATA, stdout);
    if(sdp_fp != stdout) {
        fwrite(_describe.c_str(), 1, _describe.length(), sdp_fp);
        fclose(sdp_fp);
    }
    TRACE() << "DECRIBED \n[" << _describe << "]\n";
}

void rtspcam::_rtsp_setup()
{
    char uri[400];

    snprintf(uri, sizeof(uri), "%s/%s", _url.c_str(), _control);
    CU_SET(_curl, CURLOPT_RTSP_STREAM_URI, uri);
    CU_SET(_curl, CURLOPT_RTSP_TRANSPORT, _transport.c_str());
    CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_SETUP);
    _perform();
}

void rtspcam::_rtsp_play()
{
    const char *range = "0.000-";

    CU_SET(_curl, CURLOPT_RTSP_STREAM_URI, _url.c_str());
    CU_SET(_curl, CURLOPT_RANGE, range);
    CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_PLAY);
    CU_SET(_curl, CURLOPT_WRITEDATA, this);
    CU_SET(_curl, CURLOPT_WRITEFUNCTION, rtspcam::_write_callback);
    if(_ontcp)
    {
        CU_SET(_curl, CURLOPT_INTERLEAVEDATA, this);
        CU_SET(_curl, CURLOPT_INTERLEAVEFUNCTION, rtspcam::_interleave_callback);
        CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_RECEIVE);
    }
    _perform();
    ::usleep(1000000);
    _perform();

    CU_SET(_curl, CURLOPT_RANGE, NULL);
}

void rtspcam::_rtsp_teardown()
{
    printf("\nRTSP: TEARDOWN %s\n", _url.c_str());
    CU_SET(_curl, CURLOPT_RTSP_REQUEST, (long)CURL_RTSPREQ_TEARDOWN);
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

bool rtspcam::spin()
{
    return true;
}



bool rtspcam::init(const dims_t&)
{
    return start_thread()==0;
}

#endif
