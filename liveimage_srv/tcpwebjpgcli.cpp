///////////////////////////////////////////////////////////////////////////////////////////////
#include "tcpwebjpgcli.h"
#include "logger.h"


#define    JPEG_MAGIC       0x12345678

/**
 * @brief JpegHdr  (multipart animated JPEG)
 */
static char JpegHdr[]="Content-Type: image/jpeg\r\n"
                       "Content-Length: %d\r\n"
                       "X-Timestamp: %d.%d\r\n\r\n";

static char JpegPart[] = "\r\n--thesupposexuniqueb\r\nContent-type: image/jpeg\r\n";

static char JpegPartHeader[] = "HTTP/1.0 200 OK\r\n"
                               "Connection: close\r\n"
                               "Server: vct/1.0\r\n"
                               "Cache-Control: no-cache\r\n"
                               "Content-Type: multipart/x-mixed-replace;boundary=thesupposexuniqueb\r\n"
                               "\r\n"
                               "--thesupposexuniqueb\r\n";
CliJpegSock::CliJpegSock(RawSock& o,
                 const LiFrmHdr& h):TcpWebSock(o,h)
{
    GLOGD(__FUNCTION__);
    _headered=false;
    _t = CLIJPEG;
}

/**
 * @brief CliJpegSock::~CliJpegSock
 */
CliJpegSock::~CliJpegSock()
{
    GLOGD(__FUNCTION__);
}

/**
 * @brief CliJpegSock::snd
 * @param b
 * @param rec_off
 * @return
 */
int CliJpegSock::snd(const uint8_t* b,size_t rec_off,uint32_t extra,const char* hdr)
{
    char    buffer[512] = {0};
    struct timeval timestamp;
    struct timezone tz = {5,0};

    gettimeofday(&timestamp,&tz);

    if(_headered==false)
    {
        // one time header when browserc onnatecs the <image src''> tag
        TcpWebSock::snd((const uint8_t*)JpegPartHeader,
                     strlen(JpegPartHeader),0,nullptr);
        _headered=true;
    }
    //every frame header
    size_t hl = ::sprintf(buffer,JpegHdr,
                                    (int)rec_off,
                                    (int)timestamp.tv_sec,
                                    (int)timestamp.tv_usec);

    TcpWebSock::snd((const uint8_t*)buffer,hl,0,nullptr);
    TcpWebSock::snd(b,rec_off,0,nullptr);
    TcpWebSock::snd((const uint8_t*)JpegPart,strlen(JpegPart),0,nullptr);
    return 1;
}

