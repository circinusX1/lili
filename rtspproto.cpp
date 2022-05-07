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

static constexpr const char* USR_AGENT =   "lili-1.0";
static constexpr const char* WWW_AUTH  =   "WWW-Authenticate: ";
static constexpr const char* ACCEPT    =   "Accept: ";
static constexpr const char* CONT_TYPE =   "Content-Type: ";
static constexpr const char* CONT_LEN  =   "Content-Length: ";
static constexpr const char* CONT_BASE =   "Content-Base: ";
static constexpr const char* TRANSPORT =   "Transport: ";
static constexpr const char* HDR_ABASIC = "Authorization: Basic ";
static constexpr const char* CSEQ      =   "CSeq: ";
static constexpr const char* CLI_PORT  =   "client_port=";
static constexpr const char* SRV_PORT  =   "server_port=";
static constexpr const char* SESSION   =   "Session: ";
static constexpr const char*  SDP_ATYPE =   "a=type:";
static constexpr const char*  SDP_ACTL  =   "a=control:";
static constexpr const char*  SDP_M     =   "m=";
static constexpr const char*  SDP_ARMAP =   "a=rtpmap:";
static constexpr const char*  SDP_FMT   =   "a=fmtp:";
static constexpr const char*  SDP_XDIM  =   "a=x-dimensions:";
static constexpr const char*  SDP_ADIM  =   "a=framesize:";


constexpr const char* __hdrs[] = {
    ACCEPT,
    WWW_AUTH,
    CONT_TYPE,
    CONT_LEN,
    CONT_BASE,
    TRANSPORT,
    CLI_PORT,
    SRV_PORT,
    SESSION,
    nullptr
};

constexpr const char* __sdps[] = {
    SDP_ATYPE,
    SDP_ACTL,
    SDP_M,
    SDP_ARMAP,
    SDP_FMT,
    SDP_XDIM,
    SDP_ADIM,
    nullptr
};


rtspproto::rtspproto() {

}

rtspproto::~rtspproto() {

}


void  rtspproto::_reset()
{
    _query.clear();
    _query.push_back("OPTIONS");
    _query.push_back("DESCRIBE");
    _query.push_back("SETUP");
    _query.push_back("PLAY");
    _sequence = 0;
}

void rtspproto::_send_request()
{
    if(_query.size() && _request_sent==false)
    {
        std::string request;
        const std::string& what = _query.front();
        _waitseq = _sequence;
        request += what;
        request += " ";

        if(_hdrs.find(CONT_BASE)!=_hdrs.end())
        {
            request += _hdrs[CONT_BASE];
        }
        else if(_sdps.find(SDP_ACTL)!=_sdps.end())
        {
            request += _sdps[SDP_ACTL];
        }
        else
        {
            request += _rtspurl;
        }

        request += " RTSP/1.0\r\n";
        request += "CSeq: "; request += std::to_string(_sequence); request+="\r\n";
        if(!_auth_hdr.empty())
        {
            request += _auth_hdr;
        }
        if(what == "DESCRIBE")
        {
            request += "Accept: application/sdp\r\n";
        }
        else if(what == "SETUP")
        {
            request += "Transport: RTP/AVP/UDP;unicast;client_port=";
            request += std::to_string(_client_ports[0]);
            request += "-";
            request += std::to_string(_client_ports[1]);
            request += "\r\n";
        }
        else if(what == "PLAY")
        {
            request += SESSION;
            request += _hdrs[SESSION];
            request += "\r\n";
            request += "Range: npt=0.000-\r\n";
        }
        request += USR_AGENT; request +="\r\n\r\n";
        _sequence++;
        TRACE() << "-------------------------------------\n";
        TRACE() << request;
        TRACE() << "-------------------------------------";
        _request_sent = _tcp.sendall(request.c_str(), request.length()) == int(request.length());
    }
}


bool rtspproto::_mess_response(const char *buf, size_t) {

    std::stringstream ss(buf);
    std::string line;
    bool        okays[2] = {false,false};
    int         seq = 0;

    const std::string what = _query.front();
    _request_sent = false;
    while(std::getline(ss, line, '\n') )
    {
        while(line.back()=='\r'||line.back()=='\n')
            line.pop_back();
        if(line.empty()){
            continue;
        }
        TRACE()<<"LINE: [" << line << "]\n";
        if(line.find(WWW_AUTH) != std::string::npos)
        {
            _hdrs[WWW_AUTH] = line.substr(18);
            okays[0]=true;
            okays[1]=true;
        }
        else if(line.find("200")!=std::string::npos)
        {
            okays[0]=true;
        }
        else if(line.find(CSEQ)!=std::string::npos)
        {
            ::sscanf(line.c_str(),"CSeq: %d",&seq);
            if(seq==_waitseq)
                okays[1] = true;
        }
        if(okays[0]==true &&
                okays[1]==true)
            break;
    }

    if(okays[0] && okays[1])
    {
        int   emptylines = 0;
        while(std::getline(ss, line, '\n'))
        {
            while(line.back()=='\r'||line.back()=='\n'){
                line.pop_back();
            }
            if(line.empty()){
                emptylines++;
                continue;
            }
            TRACE()<<"LINE: [" << line << "]\n";

            for(int i=0; __hdrs[i]; i++)
            {
                size_t ptok = line.find(__hdrs[i]);
                if(ptok != std::string::npos)
                {
                    std::string param = line.substr(ptok + ::strlen(__hdrs[i]));
                    size_t eop = param.find_last_of(';');
                    param = param.substr(0,eop);
                    _hdrs[ __hdrs[i] ] = param;
                    TRACE()<<"HDR: ["<< __hdrs[i] << "]=>'" << param << "'\n";
                }
            }
            if(what=="DESCRIBE" || what=="SETUP")
            {
                for(int i=0; __sdps[i]; i++)
                {
                    if(line.compare(0,::strlen(__sdps[i]),__sdps[i])==0)
                    {
                        std::string param = line.substr(::strlen(__sdps[i]));
                        _sdps[ __sdps[i] ] = param;
                        TRACE()<< "SDP: [" <<__sdps[i] << "]=>'" << param << "'\n";
                    }
                }
            }
        }
        if(what=="SETUP")
        {
            int okays = 0;
            if(!_hdrs[CLI_PORT].empty())
            {
                TRACE()<<_hdrs[CLI_PORT]<<"\n";
                okays+=sscanf(_hdrs[CLI_PORT].c_str(),"%d-%d",_client_ports,_client_ports+1);
            }
            if(!_hdrs[SRV_PORT].empty())
            {
                TRACE()<<_hdrs[SRV_PORT]<<"\n";
                okays+=sscanf(_hdrs[SRV_PORT].c_str(),"%d-%d",_server_ports,_server_ports+1);
            }
            if(okays==4)
            {
                _spawn_udp();
            }
        }

        if(_auth_hdr.empty())
        {
            const auto reqauto = _hdrs.find(WWW_AUTH);
            if(reqauto != _hdrs.end())
            {
                _authenticate();
            }
            else
            {
                _query.pop_front();
            }
        }
        else {
            _query.pop_front();
        }

    }

    if(what=="PLAY"){
        const std::map<std::string, std::string>::iterator& dims = _sdps.find(SDP_XDIM);
        if(dims != _sdps.end())
        {
            ::sscanf(dims->second.c_str(),"%d,%d",&_dims.x,&_dims.y);
        }else{

            const std::map<std::string, std::string>::iterator& dims = _sdps.find(SDP_ADIM);
            if(dims != _sdps.end())
            {
                std::string s =  dims->second.substr(3);
                ::sscanf(s.c_str(),"%d-%d",&_dims.x,&_dims.y);
            }
        }
    }

    TRACE() << "-------------------------------------";
    return true;
}


bool rtspproto::_spawn_udp()
{
    if( _create_udp(_client_ports[0], _client_ports[1]))
    {
        const unsigned char natpacket[] = {0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        return _udp.send(natpacket, sizeof(natpacket),_server_ports[0], _host.c_str() )>0;
    }
    return false;
}

bool rtspproto::_authenticate()
{
    //WWW-Authenticate: Basic realm="rtspsvc"
    if(!_credentials.empty())
    {
        std::string up = base64_encode((const unsigned char*)_credentials.c_str(), _credentials.length());
        char buf[1024];
        _auth_hdr = HDR_ABASIC;
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
                return ::strstr(buf,"200") != nullptr  ? true: false;
            }
            return false;
        }
    }
    return true;
}



void rtspproto::_do_udp(const char *buf, size_t bufsize, Frame* frame, int& index) {
    _rtp_parse((unsigned char*)buf,bufsize, frame, index);
    frame[index]._wh = _dims;
    return ;
}

bool rtspproto::cmd_play(std::string url, const std::string& credentials)
{
    int     port = 0;
    char            scheme[8];
    char            host[128];
    char            path[128];

    _rtspurl = url;
    _credentials = credentials;
    _reset();


    //signal(SIGPIPE, SIG_IGN);
    parseURL(url.c_str(), scheme,
             sizeof(scheme), host, sizeof(host),
             &port, path, sizeof(path));
    _uri = path;
    _host = host;
    if(_tcp.create(port))
    {
        if(_tcp.try_connect(host, port))
        {
            _seed();
            return true;
        }
    }
    return false;
}

bool rtspproto::spin(Frame* frame, int& index)
{
    int  r;
    char recvbuf[2048];
    static time_t now = gtc();
    static int accum = 0;

    _seed();
    r = _harvest();
    if (r & TIO_RD)
    {
        int recvbytes = _tcp.receive(recvbuf, sizeof(recvbuf));
        if (recvbytes <= 0) {
            _tcp.destroy();
            return false;
        }
        else
        {
            TRACE() << recvbuf << "\n";
            _mess_response(recvbuf, recvbytes);
        }
    }

    if (r & UIO_RD)
    {
        size_t recvbytes = _udp.receive(recvbuf, sizeof(recvbuf));
        if(recvbytes)
        {
            accum += recvbytes;
            if(gtc()-now>=1000)
            {
                TRACE() << accum << " OPS\n";
                now=gtc();
                accum=0;
            }
        }
        _do_udp(recvbuf, recvbytes, frame, index);
    }

    if (r & USIO_RD)
    {
        _udp.receive(recvbuf, sizeof(recvbuf));
    }

    if (r & TIO_WR)
    {
        _send_request();
    }

    if (r & TIO_ER)
    {
        TRACE() << "socket error\n";
        return false;
    }

    return true;
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

void rtspproto::stop()
{
    _udp.destroy();
    _udpc.destroy();
    _tcp.destroy();
}


void rtspproto::_rtp_stats_print()
{
    return ;
    printf(">> RTP Stats\n");
    printf("   First Sequence  : %u\n", _rtp_st.first_seq);
    printf("   Highest Sequence: %u\n", _rtp_st.highest_seq);
    printf("   RTP Received    : %u\n", _rtp_st.rtp_received);
    printf("   RTP Identifier  : %u\n", _rtp_st.rtp_identifier);
    printf("   RTP Timestamp   : %u\n", _rtp_st.rtp_ts);
    printf("   Jitter          : %u\n", _rtp_st.jitter);
    printf("   Last DLSR       : %i\n", _rtp_st.last_dlsr);

}

void rtspproto::_rtp_stats_update(struct rtp_header *rtp_h)
{
    uint32_t transit;
    int delta;
    struct timeval now;

    gettimeofday(&now, NULL);
    _rtp_st.rtp_received++;

    /* Highest sequence */
    if (rtp_h->seq > _rtp_st.highest_seq) {
        _rtp_st.highest_seq = rtp_h->seq;
    }


    /* Update RTP timestamp */
    if (_rtp_st.last_rcv_time.tv_sec == 0) {
        //_rtp_st.rtp_ts = rtp_h->ts;
        _rtp_st.first_seq = rtp_h->seq;
        //_rtp_st.jitter = 0;
        //_rtp_st.last_dlsr = 0;
        //_rtp_st.rtp_cum_lost = 0;
        gettimeofday(&_rtp_st.last_rcv_time, NULL);

        /* deltas
        int sec  = (rtp_h->ts / RTP_FREQ);
        int usec = (((rtp_h->ts % RTP_FREQ) / (RTP_FREQ / 8000))) * 125;
        _rtp_st.ts_delta.tv_sec  = now.tv_sec - sec;
        _rtp_st.ts_delta.tv_usec = now.tv_usec - usec;


        _rtp_st.last_arrival = rtp_tval2rtp(_rtp_st.ts_delta.tv_sec,
                                           _rtp_st.ts_delta.tv_usec);
        _rtp_st.last_arrival = rtp_tval2RTP(now);

    }
    else {*/
    }
    /* Jitter */
    transit = _rtp_st.delay_snc_last_SR;
    //printf("TRANSIT!: %i\n", transit); exit(1);
    delta = transit - _rtp_st.transit;
    _rtp_st.transit = transit;
    if (delta < 0) {
        delta = -delta;
    }
    //printf("now = %i ; rtp = %i ; delta = %i\n",
    //       t, rtp_h->ts, delta);
    //_rtp_st.jitter += delta - ((_rtp_st.jitter + 8) >> 4);
    _rtp_st.jitter += ((1.0/16.0) * ((double) delta - _rtp_st.jitter));

    _rtp_st.rtp_ts = rtp_h->ts;
    //}

    /* print the new stats */
    _rtp_stats_print();
}


int rtspproto::_streamer_write_nal(Frame* frame, int index)
{
    uint8_t nal_header[4] = {0x00, 0x00, 0x00, 0x01};
    frame[index].append(nal_header,sizeof(nal_header));
    return 1;
}

int rtspproto::_streamer_write(const void *buf, size_t count, Frame& frame, bool done)
{
    /* write to pipe */
    frame.append((const uint8_t*)buf,count);
    if(done){
        frame.ready();
    }
    return 1;
}

int rtspproto::_rtp_parse(unsigned char *raw, int size, Frame* frame, int& index)
{
    int raw_offset = 0;
    int rtp_length = size;
    int paysize;
    unsigned char payload[8912];
    struct rtp_header rtp_h;

    rtp_h.version = raw[raw_offset] >> 6;
    rtp_h.padding = CHECK_BIT(raw[raw_offset], 5);
    rtp_h.extension = CHECK_BIT(raw[raw_offset], 4);
    rtp_h.cc = raw[raw_offset] & 0xFF;

    /* next byte */
    raw_offset++;

    rtp_h.marker = CHECK_BIT(raw[raw_offset], 8);
    rtp_h.pt     = raw[raw_offset] & 0x7f;

    /* next byte */
    raw_offset++;

    /* Sequence number */
    rtp_h.seq = raw[raw_offset] * 256 + raw[raw_offset + 1];
    raw_offset += 2;

    /* time stamp */
    rtp_h.ts = \
            (raw[raw_offset    ] << 24) |
            (raw[raw_offset + 1] << 16) |
            (raw[raw_offset + 2] <<  8) |
            (raw[raw_offset + 3]);
    raw_offset += 4;

    /* ssrc / source identifier */
    rtp_h.ssrc = \
            (raw[raw_offset    ] << 24) |
            (raw[raw_offset + 1] << 16) |
            (raw[raw_offset + 2] <<  8) |
            (raw[raw_offset + 3]);
    raw_offset += 4;
    _rtp_st.rtp_identifier = rtp_h.ssrc;

    /* Payload size */
    paysize = (rtp_length - raw_offset);

    memset(payload, '\0', sizeof(payload));
    memcpy(&payload, raw + raw_offset, paysize);

    /*
     * A new RTP packet has arrived, we need to pass the rtp_h struct
     * to the stats/context updater
     */
    _rtp_stats_update(&rtp_h);
    /* Display RTP header info */
    /*
    printf("   >> RTP\n");
    printf("      Version     : %i\n", rtp_h.version);
    printf("      Padding     : %i\n", rtp_h.padding);
    printf("      Extension   : %i\n", rtp_h.extension);
    printf("      CSRC Count  : %i\n", rtp_h.cc);
    printf("      Marker      : %i\n", rtp_h.marker);
    printf("      Payload Type: %i\n", rtp_h.pt);
    printf("      Sequence    : %i\n", rtp_h.seq);
    printf("      Timestamp   : %u\n", rtp_h.ts);
    printf("      Sync Source : %u\n", rtp_h.ssrc);
*/
    /*
     * NAL, first byte header
     *
     *   +---------------+
     *   |0|1|2|3|4|5|6|7|
     *   +-+-+-+-+-+-+-+-+
     *   |F|NRI|  Type   |
     *   +---------------+
     */
    int nal_forbidden_zero = CHECK_BIT(payload[0], 7);
    int nal_nri  = (payload[0] & 0x60) >> 5;
    int nal_type = (payload[0] & 0x1F);
#if 0
    printf("      >> NAL\n");
    printf("         Forbidden zero: %i\n", nal_forbidden_zero);
    printf("         NRI           : %i\n", nal_nri);
    printf("         Type          : %i\n", nal_type);
#endif

    /* Single NAL unit packet */
    if (nal_type >= NAL_TYPE_SINGLE_NAL_MIN &&
            nal_type <= NAL_TYPE_SINGLE_NAL_MAX) {

        /* Write NAL header */

        _streamer_write_nal(frame, index);

        /* Write NAL unit */
        _streamer_write(payload, sizeof(paysize), frame[index]);
    }

    /*
     * Agregation packet - STAP-A
     * ------
     * http://www.ietf.org/rfc/rfc3984.txt
     *
     * 0                   1                   2                   3
     * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                          RTP Header                           |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |STAP-A NAL HDR |         NALU 1 Size           | NALU 1 HDR    |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                         NALU 1 Data                           |
     * :                                                               :
     * +               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |               | NALU 2 Size                   | NALU 2 HDR    |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                         NALU 2 Data                           |
     * :                                                               :
     * |                               +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     * |                               :...OPTIONAL RTP padding        |
     * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     */
    else if (nal_type == NAL_TYPE_STAP_A) {
        uint8_t *q;
        uint16_t nalu_size;

        q = payload + 1;
        int nidx = 0;

        nidx = 0;
        while (nidx < paysize - 1) {
            /* write NAL header */
            _streamer_write_nal(frame, index);

            /* get NALU size */
            nalu_size = (q[nidx] << 8) | (q[nidx + 1]);
            printf("nidx = %i ; NAL size = %i ; RAW offset = %i\n",
                   nidx, nalu_size, raw_offset);
            nidx += 2;

            /* write NALU size */
            _streamer_write(&nalu_size, 1, frame[index]);

            if (nalu_size == 0) {
                nidx++;
                continue;
            }

            /* write NALU data */
            _streamer_write(q + nidx, nalu_size, frame[index]);
            nidx += nalu_size;
        }
    }
    else if (nal_type == NAL_TYPE_FU_A) {
        //printf("         >> Fragmentation Unit\n");

        uint8_t *q;
        q = payload;

        uint8_t h264_start_bit = q[1] & 0x80;
        uint8_t h264_end_bit   = q[1] & 0x40;
        uint8_t h264_type      = q[1] & 0x1F;
        uint8_t h264_nri       = (q[0] & 0x60) >> 5;
        uint8_t h264_key       = (h264_nri << 5) | h264_type;

        if (h264_start_bit) {
            /* write NAL header */
            _streamer_write_nal(frame, index);

            /* write NAL unit code */
            _streamer_write(&h264_key, sizeof(h264_key),frame[index]);
        }
        _streamer_write(q + 2, paysize - 2,frame[index]);

        if (h264_end_bit) {
            TRACE()<< "End BIT \n";
            frame[index].ready();
            //frame[!index].reset();
        }
    }
    else if (nal_type == NAL_TYPE_UNDEFINED) {
    }
    else {
        printf("OTHER NAL!: %i\n", nal_type);
        raw_offset++;

    }
    raw_offset += paysize;

    if (rtp_h.seq > _rtp_st.highest_seq) {
        _rtp_st.highest_seq = rtp_h.seq;
    }

    _rtp_stats_print();
    return raw_offset;
}
