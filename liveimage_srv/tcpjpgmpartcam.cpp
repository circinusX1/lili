///////////////////////////////////////////////////////////////////////////////////////////////
#include "tcpjpgmpartcam.h"
#include "logger.h"
#include "main.h"

TcpJpgMpartCam::TcpJpgMpartCam(RawSock& rs,
                               const LiFrmHdr& h,const char* pextra):TcpCamCli(rs,h ,pextra)

{
    _data.reset();
}

TcpJpgMpartCam::~TcpJpgMpartCam()
{
}


int  TcpJpgMpartCam::transfer(const std::vector<RawSock*>& clis)
{
/*   brute force
    int by = this->receiveall(_data.buff,sizeof(LiFrmHdr));
    if(by == sizeof(LiFrmHdr)){
        int len = ((LiFrmHdr*)_data.buff)->len;
        by = this->receiveall(_data.buff,len);
        if(by==len){
           _deliverChunk(_data.buff,len);
           return 0;
        }
    }
    GLOGE(" execption" << by);
    this->destroy();
    throw CAM;
    return 0;
*/
    int clients = 0;
    int antilock = 32;
    int bytes = this->recdata();
    while(bytes>0 && antilock--)
    {
        if(_data.vfl==0)
        {
            if(bytes < (int) sizeof(LiFrmHdr)) { goto EXIT_;}

            LiFrmHdr* ph = (LiFrmHdr*)_data.prc_ptr();

            if(ph->magic==JPEG_MAGIC){
                _header.event = ph->event;
                _data.vfl = ph->len;
                if(ph->event.predicate & EVT_KEEP_ALIVE){
                    _nowt = SECS();
                }
                _data.advance_prc(sizeof(LiFrmHdr));
                bytes -= sizeof(LiFrmHdr);
            }
            else // we got out os sync
            {
                GLOGE("critical cannot happen. we got out of sync");
                this->destroy();
                throw CAM;
            }
        }
        else
        {
            if(bytes >= (int)_data.vfl)
            {
                 clients = _deliverChunk(_data.prc_ptr(),_data.vfl);
                 _data.outbytes += _data.vfl + sizeof(LiFrmHdr);
                 _data.advance_prc(_data.vfl);
                 bytes -= _data.vfl;
                 int vfl = _data.vfl;
                 _data.vfl = 0;
                if(_data.room() <= vfl - _data.left()){
                    if(bytes)
                        ::memmove(_data.buff,_data.prc_ptr(),bytes);
                    _data.prc_off = 0;
                    _data.rec_off = bytes;
                }
            }
            else{
                if(_data.room() > _data.vfl - _data.left())
                {
                    break;
                }
                if(bytes)
                    ::memmove(_data.buff,_data.prc_ptr(),bytes);
                _data.prc_off = 0;
                _data.rec_off = bytes;
                break;
            }
        }
    }
EXIT_:
    if(antilock==0){
        GLOGE("critical we had a hang");
        _data.print("critical");
        this->destroy();
        throw CAM;
    }
    if(_header.insync){
        can_send(true);
        _ask_frame = 1;
    }
    if(clients){
        _nowt = SECS();
    }
    return bytes;
}

