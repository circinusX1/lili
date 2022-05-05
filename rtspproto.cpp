//
//  rtspproto.cpp
//  toolForTest
//
//  Created by cx on 2018/9/6.
//  Copyright © 2018年 cx. All rights reserved.
//
#include <string.h>
#include "rtspproto.h"
#include <unistd.h>

#define MODULE_TAG "rtspproto"

#define log(tag,fmt,...)\
    do {\
    printf("%s:", tag);\
    printf(fmt, ##__VA_ARGS__);\
    printf("\n");\
    } while(0)

#define SetNextState(x) _PlayState = x;

#define VIDEO_RTP_PORT (12000)
#define VIDEO_RTCP_PORT (12001)

static constexpr char base64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                       "abcdefghijklmnopqrstuvwxyz"
                                       "0123456789+/";

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len)
{
    std::string ret;
    int i = 0;
    int j = 0;
    unsigned char char_array_3[3];
    unsigned char char_array_4[4];

    while (in_len--) {
        char_array_3[i++] = *(bytes_to_encode++);
        if (i == 3) {
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            char_array_4[3] = char_array_3[2] & 0x3f;

            for(i = 0; (i <4) ; i++)
                ret += base64_chars[char_array_4[i]];
            i = 0;
        }
    }

    if (i)
    {
        for(j = i; j < 3; j++)
            char_array_3[j] = '\0';

        char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
        char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
        char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
        char_array_4[3] = char_array_3[2] & 0x3f;

        for (j = 0; (j < i + 1); j++)
            ret += base64_chars[char_array_4[j]];

        while((i++ < 3))
            ret += '=';

    }
    return ret;
}

rtspproto::rtspproto() {
    _Terminated = false;
    _NetWorked = false;
    _PlayState = RtspIdle;
}

rtspproto::~rtspproto() {
    Stop();
}

bool rtspproto::getIPFromUrl(std::string url, char *ip, int *port) {
    unsigned int dstip[4] = {0};
    int dstport = 0;
    int field = sscanf(url.c_str(), "rtsp://%d.%d.%d.%d:%d", &dstip[0], &dstip[1], &dstip[2], &dstip[3], &dstport);
    if (field < 4) {
        log(MODULE_TAG, "failed to get ip from url");
        return false;
    } else if (field == 4) {
        sprintf(ip, "%d.%d.%d.%d", dstip[0], dstip[1], dstip[2], dstip[3]);
        *port = dstport = 554;
    } else if (field == 5) {
        sprintf(ip, "%d.%d.%d.%d", dstip[0], dstip[1], dstip[2], dstip[3]);
        *port = dstport;
    } else {
        log(MODULE_TAG, "failed to get ip from url");
        return false;
    }

    return true;
}

bool rtspproto::NetworkInit(const char *ip, const short port) {
    if(_tcp.create(port))
    {
        _NetWorked = _tcp.try_connect(ip, port);
    }
    return _NetWorked;
}

bool rtspproto::RTPSocketInit(int videoPort) {
    if (videoPort) {
        return _create_udp(videoPort, videoPort+1);
    }
    return false;
}

void rtspproto::EventInit() {
    _seed();
}

std::vector<std::string> rtspproto::GetSDPFromMessage(const char *buffer, size_t length, const char *pattern) {
    char *tempBuffer = (char *)malloc(length + 1);
    strcpy(tempBuffer, buffer);

    std::vector<std::string> rvector;
    char* tmpStr = strtok(tempBuffer, pattern);
    while (tmpStr != NULL)
    {
        rvector.push_back(std::string(tmpStr));
        tmpStr = strtok(NULL, pattern);
    }
    free(tempBuffer);
    return rvector;
}

void rtspproto::SendDescribe(std::string url) {
    char buf[1024];
    sprintf(buf, "DESCRIBE %s RTSP/1.0\r\n"
                 "Accept: application/sdp\r\n"
                 "CSeq: %d\r\n"
                 "%s"
                 "User-Agent: Lavf58.12.100\r\n"
                 "\r\n",
                    url.c_str(),
                   RTSPDESCRIBE,
                   _auth_hdr.c_str());
    TRACE()<<buf << "\n";
    _tcp.sendall( buf, strlen(buf));
}

void rtspproto::HandleDescribe(const char *buf, ssize_t bufsize)
{
    std::vector<std::string> rvector = GetSDPFromMessage(buf, bufsize, "\r\n");
    std::string sdp;
    for (auto substr : rvector) {
        if (strstr(substr.c_str(),"Session:")) {
            ::sscanf(substr.c_str(), "Session:%ld", &_RtspSessionID);
        } else if (strchr(substr.c_str(), '=')) {
            sdp.append(substr);
            sdp.append("\n");
        }
    }
    _sdper = ::sdp_parse(sdp.c_str());

    struct sdp_media* pm = _sdper->medias;
    if(pm->attributes && *pm->attributes)
    {
        const char* contr = ::strstr(*pm->attributes,"control:");
        if(contr)
        {
            _uri = contr + 8;
        }
    }
    TRACE() << _sdper->information << "\n";
}

void rtspproto::RtspSetup(const std::string url,
                          int track, int CSeq, char *proto,
                          short rtp_port, short rtcp_port)
{
    char buf[1024];
    sprintf(buf, "SETUP %s RTSP/1.0\r\n"
                 "CSeq: %d\r\n"
                 "User-Agent: Lavf58.12.100\r\n"
                 "Transport: %s;unicast;client_port=%d-%d\r\n"
                 "%s"
                 "\r\n", _uri.c_str(),
                 CSeq, proto, rtp_port, rtcp_port,_auth_hdr.c_str());
    TRACE()<<buf << "\n";
    _tcp.sendall( buf, strlen(buf));
}

void rtspproto::SendVideoSetup() {
    int i = 0, j = 0;
    int videoTrackID = 0;
    for (i = 0; i < _sdper->medias_count; i++) {
        if (strcmp(_sdper->medias[i].info.type, "video") == 0) {
            for (j = 0; j < _sdper->medias[i].attributes_count; j++) {
                if (strstr(_sdper->medias[i].attributes[j], "trackID")) {
                    ::sscanf(_sdper->medias[i].attributes[j], "control:trackID=%d", &videoTrackID);
                }
            }
            RtspSetup(_rtspurl, videoTrackID, RTSPVIDEO_SETUP, _sdper->medias[i].info.proto,
                      VIDEO_RTP_PORT, VIDEO_RTCP_PORT);
        }
    }
}

bool rtspproto::HandleVideoSetup(const char *buf, ssize_t bufsize)
{
    int rtp_port = 0;
    int rtcp_port = 0;
    int remote_port = 0;
    int remote_rtcp_port = 0;

    if (strstr(buf, "client_port=")) {
        ::sscanf(strstr(buf, "client_port="), "client_port=%d-%d", &rtp_port, &rtcp_port);
    }

    if(strstr(buf, "server_port=")) {
        ::sscanf(strstr(buf, "server_port="), "server_port=%d-%d", &remote_port, &remote_rtcp_port);
    }

    if (!RTPSocketInit(rtp_port ? rtp_port : VIDEO_RTP_PORT)) {
        log(MODULE_TAG, "rtp socket init failed");
        return false;
    }

    SADDR_46 remoteAddr;
    remoteAddr.sin_family = AF_INET;
    remoteAddr.sin_port = htons(remote_port);
    remoteAddr.sin_addr.s_addr = inet_addr(_rtspip);

    const unsigned char natpacket[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
    return _udp.send(natpacket, sizeof(natpacket), remoteAddr)>0;

}

void rtspproto::SendPlay(const std::string url) {
    char buf[1024];
    sprintf(buf, "PLAY %s RTSP/1.0\r\n"
                 "CSeq: %u\r\n"
                 "Session: %s\r\n"
                 "%s"
                 "Range: npt=0.000-\r\n" // Range
                 "User-Agent: Lavf58.12.100\r\n"
                 "\r\n", url.c_str(), RTSPPLAY,
                        _session.c_str(), _auth_hdr.c_str());
    TRACE()<<buf<<"\n";
    _tcp.sendall(buf, strlen(buf));
}

bool rtspproto::_authenticate(const char* authline)
{
    //WWW-Authenticate: Basic realm="rtspsvc"
    if(!_credentials.empty())
    {
        std::string up = base64_encode((const unsigned char*)_credentials.c_str(), _credentials.length());
        char buf[1024];
        _auth_hdr = "Authorization: Basic ";
        _auth_hdr += up.c_str();
        _auth_hdr += "\r\n";

        sprintf(buf, "OPTIONS * RTSP/1.0\r\n"
                     "%s"
                     "\r\n", _auth_hdr.c_str());
        TRACE()<<buf<<"\n";
        int bytes =  _tcp.sendall(buf, strlen(buf));
        if(bytes>0)
        {
            bytes = _tcp.receive(buf, sizeof(buf));
            if(bytes>0)
            {
                TRACE()<<buf<<"\n";
                return ::strstr(buf,"200") > 0  ? true: false;
            }
            return false;
        }
    }
    return true;
}

bool rtspproto::HandleRtspMsg(const char *buf, ssize_t bufsize) {

    int MsgType = 0;
    bool okay = true;
    TRACE() << buf << "\n";

    if (::sscanf(buf, "%*[^C]CSeq:%d", &MsgType) != 1) {
        log(MODULE_TAG, "invalid rtsp message");
        return false;
    }
    const char* psess = ::strstr(buf,"Session: ");
    if (psess){
        char sess[128];
        ::strncpy(sess, psess+9, sizeof(sess)-1);
        char* peos = ::strchr(sess,';');
        if(peos==0)peos = ::strchr(sess,'\r');
        if(peos){
            *peos='\0';
            _session = sess;
        }
    }


    const char* pauth = ::strstr(buf,"WWW-Authenticate: ");
    if(pauth)
    {
        okay = _authenticate(pauth+18);
        if(okay)
        {
            _PlayState = RtspSendDescribe;
            MsgType = 0;
        }
    }

    if(okay){
        switch (MsgType) {
        case RTSPOPTIONS:
            break;
        case RTSPDESCRIBE:
            HandleDescribe(buf, bufsize);
            SetNextState(RtspSendVideoSetup);
            break;
        case RTSPVIDEO_SETUP:
            if (HandleVideoSetup(buf, bufsize)) {
                SetNextState(RtspSendPlay);
            }
            break;
        case RTSPAUDIO_SETUP:
            break;
        case RTSPPLAY:
            break;
        default:
            log(MODULE_TAG, "unknow rtsp message");
            break;
        }
        return true;
    }
    return false;
}

void rtspproto::HandleRtspState() {
    switch (_PlayState.load()) {
    case RtspSendOptions:
        log(MODULE_TAG, "rtsp send options");
        break;
    case RtspHandleOptions:
        log(MODULE_TAG, "rtsp handle options");
        break;
    case RtspSendDescribe:
        log(MODULE_TAG, "rtsp send describe");
        SendDescribe(_rtspurl);
        break;
    case RtspHandleDescribe:
        log(MODULE_TAG, "rtsp handle describe");
        break;
    case RtspSendVideoSetup:
        log(MODULE_TAG, "rtsp send video setup");
        SendVideoSetup();
        break;
    case RtspHandleVideoSetup:
        log(MODULE_TAG, "rtsp handle video setup");
        break;
    case RtspSendAudioSetup:
        log(MODULE_TAG, "rtsp send audio setup");
        break;
    case RtspHandleAudioSetup:
        log(MODULE_TAG, "rtsp handle audio setup");
        break;
    case RtspSendPlay:
        log(MODULE_TAG, "rtsp send play");
        SendPlay(_rtspurl);
        break;
    case RtspHandlePlay:
        log(MODULE_TAG, "rtsp handle play");
        break;
    case RtspSendPause:
        log(MODULE_TAG, "rtsp send pause");
        break;
    case RtspHandlePause:
        log(MODULE_TAG, "rtsp handle pause");
        break;
    case RtspIdle:
        break;
    default:
        log(MODULE_TAG, "unkonw rtsp state");
        break;
    }

    SetNextState(RtspIdle);
}

#define RTP_OFFSET (12)
#define FU_OFFSET (14)

void rtspproto::HandleRtpMsg(const char *buf, ssize_t bufsize, Frame& frame) {
    char header[] = {0, 0, 0, 1};
    struct Nalu nalu = *(struct Nalu *)(buf + RTP_OFFSET);

    if (nalu.type >= 0 && nalu.type < 24) { //one nalu
        frame.append((const uint8_t*)header,4);
        frame.append((const uint8_t*)(buf + RTP_OFFSET),bufsize - RTP_OFFSET);
    } else if (nalu.type == 28) { //fu-a slice
        struct FU fu;
        char in = buf[RTP_OFFSET + 1];
        fu.S = in >> 7;
        fu.E = (in >> 6) & 0x01;
        fu.R = (in >> 5) & 0x01;
        fu.type = in & 0x1f;

        if (fu.S == 1) {
            char naluType = nalu.forbidden_zero_bit << 7 | nalu.nal_ref_idc << 5 | fu.type;

            frame.append((const uint8_t*)header,4);
            frame.append((const uint8_t*)&naluType,1);
            frame.append((const uint8_t*)(buf + FU_OFFSET), bufsize - FU_OFFSET);
        } else if (fu.E == 1) {
            frame.append((const uint8_t*)(buf + FU_OFFSET), bufsize - FU_OFFSET);

        } else {
            frame.append((const uint8_t*)(buf + FU_OFFSET), bufsize - FU_OFFSET);
        }
    }
}

bool rtspproto::cmd_play(std::string url,const std::string& credentials) {
    char ip[256];
    int port = 0;
    _rtspurl = url;
    _credentials = credentials;

    if (!getIPFromUrl(url, ip, &port)) {
        log(MODULE_TAG, "get ip and port failed");
        return false;
    }
    ::memcpy(_rtspip, ip, sizeof(ip));

    if (!NetworkInit(ip, port)) {
        log(MODULE_TAG, "network uninitizial");
        return false;
    }

    EventInit();
    Frame f;
    spin(f);

    return true;
}

void rtspproto::Stop() {
    _Terminated = true;
    _PlayState = RtspTurnOff;
}

void rtspproto::spin(Frame& frame)
{
    int  r;
    char recvbuf[2048];

    _seed();
    r = _harvest();
    if (r & TIO_RD)
    {
        int recvbytes = _tcp.receive(recvbuf, sizeof(recvbuf));
        if (recvbytes <= 0) {
            _tcp.destroy();
            _described = false;
            return;
        }
        else
        {
            TRACE() << recvbuf << "\n";
            if (!HandleRtspMsg(recvbuf, recvbytes))
            {
                log(MODULE_TAG, "failed to handle rtsp msg");
            }
        }
    }

    if (r & UIO_RD)
    {
        ssize_t recvbytes = _udp.receive(recvbuf, sizeof(recvbuf));
        log(MODULE_TAG, "recv rtp video packet %ld bytes", recvbytes);
        HandleRtpMsg(recvbuf, recvbytes, frame);
    }

    if (r & USIO_RD)
    {
        ssize_t recvbytes = _udp.receive(recvbuf, sizeof(recvbuf));
        log(MODULE_TAG, "recv rtp audio packet %ld bytes", recvbytes);
    }

    if (r & TIO_WR)
    {
        if(!_described)
        {
            log(MODULE_TAG, "async connect success");
            SetNextState(RtspSendDescribe);
            _described = true;
        }
    }

    if (r & TIO_ER)
    {
        log(MODULE_TAG, "event error occur");
    }

    HandleRtspState();
}


void    rtspproto::_seed(){
    FD_ZERO(&_readfd);
    FD_ZERO(&_writefd);
    FD_ZERO(&_errorfd);
    if(_tcp.isopen()){
        _maxfd = std::max(_maxfd, _tcp.socket());
        FD_SET(_tcp.socket(), &_readfd);
        FD_SET(_tcp.socket(), &_writefd);
        FD_SET(_tcp.socket(), &_errorfd);
    }
    if(_udp.isopen()){
        _maxfd = std::max(_maxfd, _udp.socket());
        FD_SET(_udp.socket(), &_readfd);
        FD_SET(_udp.socket(), &_writefd);
        FD_SET(_udp.socket(), &_errorfd);
    }
    if(_udpc.isopen()){
        _maxfd = std::max(_maxfd, _udpc.socket());
        FD_SET(_udpc.socket(), &_readfd);
        FD_SET(_udpc.socket(), &_writefd);
        FD_SET(_udpc.socket(), &_errorfd);
    }
}
int rtspproto::_harvest()
{
    struct timeval tv = {0,10000};
    int r = ::select(_maxfd + 1, &_readfd, &_writefd, &_errorfd, &tv);
    if(r < 0){
        TRACE()<< "network system error \n";
        return -1;
    }
    else if(r>0)
    {
        r = 0;
        if(_udpc.isopen())
        {
            r |= FD_ISSET(_udpc.socket(), &_readfd)  ? USIO_RD : 0;
            r |= FD_ISSET(_udpc.socket(), &_writefd) ? USIO_WR : 0;
            r |= FD_ISSET(_udpc.socket(), &_errorfd) ? USIO_ER : 0;
        }
        if(_udp.isopen())
        {
            r |= FD_ISSET(_udp.socket(), &_readfd)  ? UIO_RD : 0;
            r |= FD_ISSET(_udp.socket(), &_writefd) ? UIO_WR : 0;
            r |= FD_ISSET(_udp.socket(), &_errorfd) ? UIO_ER : 0;
        }
        if(_tcp.isopen())
        {
            r |= FD_ISSET(_tcp.socket(), &_readfd)  ? TIO_RD  : 0;
            r |= FD_ISSET(_tcp.socket(), &_writefd) ? TIO_WR  : 0;
            r |= FD_ISSET(_tcp.socket(), &_errorfd) ? TIO_ER  : 0;
        }
        return r;
    }
    return 0;
}

bool   rtspproto::_create_udp(int port, int portc)
{
    if(!_udp.create(port))
    {
        return false;
    }
    if(!_udp.bind(0,port))
    {
        return false;
    }
    if(!_udpc.create(portc))
    {
        return false;
    }
    if(!_udpc.bind(0,portc))
    {
        return false;
    }
    return true;
}
