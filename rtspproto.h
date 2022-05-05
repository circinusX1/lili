//
//  rtspproto.hpp
//  toolForTest
//
//  Created by cx on 2018/9/6.
//  Copyright © 2018年 cx. All rights reserved.
//

#ifndef RtspPlayer_hpp
#define RtspPlayer_hpp

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include "sock.h"
#include "lilitypes.h"
#include "cbconf.h"

extern "C" {
#include "sdp.h"
}


enum RtspPlayerState {
    RtspSendOptions = 0,
    RtspHandleOptions,
    RtspSendDescribe,
    RtspHandleDescribe,
    RtspSendVideoSetup,
    RtspHandleVideoSetup,
    RtspSendAudioSetup,
    RtspHandleAudioSetup,
    RtspSendPlay,
    RtspHandlePlay,
    RtspSendPause,
    RtspHandlePause,
    RtspSendTerminate,
    RtspHandleTerminate,
    RtspIdle,
    RtspTurnOff,
};

enum RtspPlayerCSeq {
    RTSPOPTIONS = 1,
    RTSPDESCRIBE,
    RTSPVIDEO_SETUP,
    RTSPAUDIO_SETUP,
    RTSPPLAY,
    RTSPPAUSE,
    RTSPTEARDOWN,
};

// h264 nalu
struct Nalu {
    unsigned type :5;
    unsigned nal_ref_idc :2;
    unsigned forbidden_zero_bit :1;
};

// h264 rtp fu
struct FU {
    unsigned type :5;
    unsigned R :1;
    unsigned E :1;
    unsigned S :1;
};

constexpr int TIO_RD      = 0x1;
constexpr int TIO_WR      = 0x2;
constexpr int TIO_ER      = 0x4;
constexpr int UIO_RD      = 0x8;
constexpr int UIO_WR      = 0x10;
constexpr int UIO_ER      = 0x20;
constexpr int USIO_RD     = 0x40;
constexpr int USIO_WR     = 0x80;
constexpr int USIO_ER     = 0x100;



class rtspproto {
public:
    typedef std::shared_ptr<rtspproto> Ptr;
    rtspproto();
    ~rtspproto();
    bool cmd_play(std::string url, const std::string& user);
    void Stop();
    void spin(Frame& frame);
protected:
    bool NetworkInit(const char *ip, const short port);
    bool RTPSocketInit(int videoPort);
    bool getIPFromUrl(std::string url, char *ip, int *port);
    void EventInit();
    bool HandleRtspMsg(const char *buf, ssize_t bufsize);
    void HandleRtspState();

    void HandleRtpMsg(const char *buf, ssize_t bufsize, Frame& frame);

    // rtsp message send/handle function
    void SendDescribe(std::string url);
    void HandleDescribe(const char *buf, ssize_t bufsize);
    void RtspSetup(const std::string url, int track, int CSeq, char *proto, short rtp_port, short rtcp_port);
    void SendVideoSetup();
    bool HandleVideoSetup(const char *buf, ssize_t bufsize);
    void SendPlay(const std::string url);
    std::vector<std::string> GetSDPFromMessage(const char *buffer, size_t length, const char *pattern);
    bool _authenticate(const char* authline);
    void    _seed();
    int    _harvest();
    bool   _create_udp(int port, int);



private:
    std::atomic<bool> _Terminated;
    std::atomic<bool> _NetWorked;
    std::atomic<RtspPlayerState> _PlayState;
    std::string _auth_hdr;

    std::string _rtspurl;
    std::string _uri;
    std::string _credentials;
    std::string _session;
    char _rtspip[256];

    int _Eventfd = 0;
    int _RtpVideoSocket = 0;
    int _RtpAudioSocket = 0;
    sockaddr_in _RtpVideoAddr;

    sdp_payload *_sdper;

    long _RtspSessionID = 0;
    //std::function<void(unsigned char *nalu, ssize_t size)> onVideoFrameGet;

    FILE *_fp;


    bool            _described=false;
    tcp_cli_sock    _tcp;
    udp_sock        _udp;
    udp_sock        _udpc;
    fd_set          _readfd;
    fd_set          _writefd;
    fd_set          _errorfd;
    int             _maxfd = 0;

};

#endif /* RtspPlayer_hpp */
