///////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <vector>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pipefile.h"
#include "cbconf.h"

//////////////////////////////////////////////////////////////////////////////////////////////////*
pipiefile::pipiefile(std::string file):_fd(0),_print(false)
{
    std::string pipefile = file;
    std::vector<std::string>  fn;

    if(::access(pipefile.c_str(),0)==0)
    {
        ::unlink(pipefile.c_str());
    }

    int fi = ::mkfifo(pipefile.c_str(),O_RDWR|O_NONBLOCK| S_IRWXU|S_IRWXG|S_IRWXG  );
    if(fi<0)
    {
        perror("mkfifo");
        return;
    }
    _fd = ::open (pipefile.c_str(),O_RDWR|O_CREAT);
    if(_fd<0)
    {
        TRACE() << file << ": PIPE: " << strerror(errno);
    }
    else
    {
        _fn = pipefile;
        ::fcntl(_fd,F_SETFL,O_NONBLOCK);
        ::fcntl(_fd,F_SETPIPE_SZ,1024 * 8912);
    }
    if(!_print)
    {
        TRACE() << "new pipe: "<< _fn << "\n";
        _print=true;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
pipiefile::~pipiefile()
{
    if(_fd)
        ::close(_fd);
    ::unlink(_fn.c_str());
    TRACE() << "delete pipe: "<< _fn << "\n";
}

///////////////////////////////////////////////////////////////////////////////////////////////////
static int LogOften=0;
int pipiefile::stream(const uint8_t* buff,size_t maxsz)
{
    if(_fd & maxsz)
    {
        size_t rv;
        size_t sent = 0;
        do{
            rv =  ::write(_fd,buff+sent,maxsz-sent);
            if(rv==std::string::npos)
            {
                //std::cerr <<__FUNCTION__ <<": "<< strerror(errno) << "\n";
                break;
            }
            sent+=rv;
        }while(sent<maxsz);
        if(++LogOften%100==0)
        {
            if(sent==maxsz){
                TRACE() << "mplayer " << _fn << " -cache 200 -cache-min 80  # has "<<maxsz<<" bytes \n";
            }
            else{
                //std::cout << " pipe full ! " << _fn << " "<<maxsz<<" bytes \n";
            }
        }
        return sent;
    }
    return 0;
}

