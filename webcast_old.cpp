#include "webcast.h"
#include "liconfig.h"


/////////////////////////////////////////////////////////////////////////////////////////////////
#define TIME_FOR_CLI    5
#define BOUNDARY        "----212367691630846570233068106084"
/////////////////////////////////////////////////////////////////////////////////////////////////
WebCast::WebCast()
{

}

////////////////////////////////////////////////////////////////////////////////////////////////
WebCast::~WebCast()
{

}

#if 0
POST / HTTP/1.1
Host: localhost:8000
Content-Type: multipart/form-data; boundary=---------------------------212367691630846570233068106084
Content-Length: 453
Connection: keep-alive

        -----------------------------16645979773237131345183406155
        Content-Disposition: form-data; name="my_file0"; filename="f1"
        Content-Type: application/octet-stream

        sssssssssssssssssssssss

        -----------------------------16645979773237131345183406155
        Content-Disposition: form-data; name="my_file1"; filename="f2"
        Content-Type: application/octet-stream

        22222222222222222222222

        -----------------------------16645979773237131345183406155--

#endif

/////////////////////////////////////////////////////////////////////////////////////////////////
void WebCast::thread_main()
{
    std::string     web = GCFG->_glb.webcast;
    time_t          ctime = 0;
    tcp_cli_sock    cli;

    while(!this->OsThread::is_stopped())
    {
        if(has && time(0)-ctime < 5)
        {
            ctime = time(0);
            cli.destroy();
            if(cli.create(80))
            {
                if(0==cli.try_connect(web.c_str(), 80))
                {
                    cli.destroy();
                }

            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
void WebCast::stream_frame(uint8_t* pjpg, size_t length)
{
    AutoLock a(&_mut);

    if(_frame==nullptr){
        _frame = new uint8_t[length*2];
    }
    ::memcpy(_frame,pjpg,length);
    _length = length;
}

/////////////////////////////////////////////////////////////////////////////////////////////////
bool WebCast::_post_multipart()
{
    struct      timeval timestamp;
    struct      timezone tz = {5,0};
    char        buffer[256];
    std::string hdr;
    static      int incr=10;

    ::gettimeofday(&timestamp, &tz);
    if(!_headered)
    {
        // where to post is coming from server
        hdr =  "POST /";
        hdr += GCFG->_glb.wdoc;
     //   hdr += "?K=";
     //   hdr += _upload_page;
        hdr += " HTTP/1.0\r\n";
        if((!GCFG->_glb.url.empty()))
        {
            //hdr += "Host: "; hdr += GCFG->_glb.url + "\r\n";
        }
        hdr += "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8\r\n";
        hdr += "Accept-Encoding: gzip, deflate\r\n";
        //hdr += "Liveimage: "; hdr += GCFG->_glb.webcastname + "\r\n";
        hdr += "Content-Type: multipart/form-data; ";
        hdr += "boundary=";
        hdr += BOUNDARY;
        hdr += "\r\n";
        hdr += "Content-Length:"; hdr +=std::to_string(_length); hdr += "\r\n";
        //hdr += "Connection: close\r\n";
        hdr += "Connection: keep-alive\r\n";
        hdr += "\r\n--";
        hdr += BOUNDARY;
        hdr += "\r\n";
        _headered = true;
    }
    hdr += "Content-Disposition: form-data; name=\"file";
    hdr += std::to_string(incr++);
    hdr += "\"; filename=\"S" ;
    hdr += std::to_string(incr++);
    hdr+="\"\r\n";
    hdr += "Content-Type: application/octet-stream\r\n";
    hdr += "\r\n";

    std::cout << hdr ;
    int sent = cli.sendall(hdr.c_str(), hdr.length());
    if(sent == (int)hdr.length())
    {
        std::cout << "[IMAGE "<<_length<<"]" ;
        sent=cli.sendall(_frame, _length);
        if(sent == (int)_length)
        {
            hdr = "\r\nboundary=--";
            hdr += BOUNDARY;
            hdr += "\r\n";
            if(cli.sendall(hdr.c_str(),hdr.length())==(int)hdr.length())
            {
                std::cout << hdr << "\r\n" ;
                return true;
            }
            else
            {
                std::cout << "????????[IMAGE FAILED BOUNDARY] ??? " ;
            }
        }
        else
        {
            std::cout << "????????[IMAGE FAILED BUFFER] ??? " ;
        }
    }
    std::cout << "[HEADER FAILED BUFFER] ??? " ;
    cli.destroy();
    _headered=false;
    msleep(64);
    return false;
}


bool WebCast::_has_clients()
{
    if(time(0) - _last_clicheck > TIME_FOR_CLI)
    {
        std::cout <<__FUNCTION__<<"\r\n";

        _last_clicheck = time(0);
        _upload_page.clear();
        std::string get = "GET /camera.php HTTP/1.0\r\n";
        if((!GCFG->_glb.url.empty()))
        {
            get += "Host: "; get += GCFG->_glb.url + "\r\n";
        }
        get += "Liveimage: "; get += GCFG->_glb.webcastname + "\r\n";
        get += "Connection: close\r\n\r\n";
        int sent = cli.sendall(get.c_str(), get.length());
        std::cout << get ;
        if(sent == (int)get.length())
        {
            char rec[1024];
            int bytes = cli.select_receive(rec,sizeof(rec)-1, 8000);
            if(bytes>0)
            {
                std::cout <<"SETUP GOT:\r\n[" << rec << "]\r\n";
                rec[bytes]='\0';
                char *eoh = ::strstr(rec, "\r\n\r\n");
                if(eoh)
                {
                    const char* data = eoh+4;
                    ::strcpy(rec,eoh);
                    eoh = ::strstr(rec, "\r\n");
                    if(eoh){
                        *eoh='\0';
                    }
                    _upload_page = data;
                }
            }
            else
            {
                std::cout <<"REC FAILED\r\n";
                cli.destroy();
            }
        }
        else
        {
            std::cout <<"SENT FAILED\r\n";
            cli.destroy();
        }
    }
    return _upload_page.length()>5;
}
