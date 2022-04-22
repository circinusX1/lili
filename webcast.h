#ifndef WEBCAST_H
#define WEBCAST_H

#include "os.h"
#include "sock.h"
#include "sockserver.h"
#include "encrypter.h"
#include "imgsink.h"
#include "lilitypes.h"



class webcast : public osthread, public imgsink
{
    enum eHOW{
        e_offline,
        e_tcp,
        e_udp,
        e_rtp,
    };

public:
    webcast(const std::string& name);
    virtual ~webcast();
    virtual void thread_main();
    virtual void stream(const uint8_t* pb, size_t len, const dims_t& imgsz,
                        const std::string& name, const event_t& event, EIMG_FMT eift);
    void kill();
    virtual bool spin(event_t& event);
    virtual bool init(const dims_t&);

private:
    void _go_streaming(const char* host, int port);

private:

    mutex           _mut;
    uint8_t*        _frame  = nullptr;
    bool            _headered = false;
    time_t          _last_clicheck=0;
    tcp_cli_sock    _s;
    LiFrmHdr        _hdr;
    size_t          _buffsz=0;
    int             _cast_fps = 1;
    int             _pool_intl = 10;
    uint32_t        _srv_key = 0;
    std::string     _security;
    Encryptor       _enc;
};

inline int parseURL(const char* url, char* scheme, size_t
                    maxSchemeLen, char* host, size_t maxHostLen,
                    int* port, char* path, size_t maxPathLen) //Parse URL
{
    (void)maxPathLen;
    char* schemePtr = (char*) url;
    char* hostPtr = (char*) strstr(url, "://");
    if(hostPtr == NULL)
    {
        printf("Could not find host");
        return 0; //URL is invalid
    }

    if( maxSchemeLen < (size_t)(hostPtr - schemePtr + 1 )) //including NULL-terminating char
    {
        printf("Scheme str is too small (%zu >= %zu)", maxSchemeLen,
               hostPtr - schemePtr + 1);
        return 0;
    }
    memcpy(scheme, schemePtr, hostPtr - schemePtr);
    scheme[hostPtr - schemePtr] = '\0';

    hostPtr+=3;

    size_t hostLen = 0;

    char* portPtr = strchr(hostPtr, ':');
    if( portPtr != NULL )
    {
        hostLen = portPtr - hostPtr;
        portPtr++;
        if( sscanf(portPtr, "%d", port) != 1)
        {
            printf("Could not find port");
            return 0;
        }
    }
    else
    {
        *port=80;
    }
    char* pathPtr = strchr(hostPtr, '/');
    if( hostLen == 0 )
    {
        hostLen = pathPtr - hostPtr;
    }

    if( maxHostLen < hostLen + 1 ) //including NULL-terminating char
    {
        printf("Host str is too small (%zu >= %zu)", maxHostLen, hostLen + 1);
        return 0;
    }
    memcpy(host, hostPtr, hostLen);
    host[hostLen] = '\0';

    size_t pathLen;
    char* fragmentPtr = strchr(hostPtr, '#');
    if(fragmentPtr != NULL)
    {
        pathLen = fragmentPtr - pathPtr;
    }
    else
    {
        if(pathPtr)
            pathLen = strlen(pathPtr);
        else
            pathLen=0;
    }

    if(pathPtr)
    {
        memcpy(path, pathPtr, pathLen);
        path[pathLen] = '\0';
    }
    else
    {
        path[0]=0;
    }

    return 1;
}

#endif // WEBCAST_H
