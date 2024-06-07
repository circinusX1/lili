///////////////////////////////////////////////////////////////////////////////////////////////
#include <iostream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "fpipe.h"
#include "main.h"
#include "sheller.h"

/**
 * @brief Fpip::Fpip
 * @param file
 */
Fpip::Fpip(std::string file):_fd(0),_print(false)
{
    std::string pipefile = __files_recs;
    std::vector<std::string>  fn;
    split(file,'-',fn);

    if(fn.size())
    {
        pipefile += fn[0]+"/";
    }
    else
    {
        pipefile += "NOMAC/";
    }

    if(::access(pipefile.c_str(),0)!=0)
    {
        ::mkdir(pipefile.c_str(),0777);
    }
    if(fn.size()==2)
        pipefile+=fn[1];

    if(::access(pipefile.c_str(),0)==0)
    {
        ::unlink(pipefile.c_str());
    }
    else
    {
        int fi = ::mkfifo(pipefile.c_str(),O_RDWR|O_NONBLOCK| S_IRWXU|S_IRWXG|S_IRWXG  );
        if(fi<0)
        {
            perror("mkfifo");
            return;
        }
    }
    _fd = ::open (pipefile.c_str(),O_RDWR|O_CREAT);
    if(_fd<0)
    {
        GLOGW( file << ": PIPE: " << strerror(errno));
    }
    else
    {
        _fn = pipefile;
        ::fcntl(_fd,F_SETFL,O_NONBLOCK);
        ::fcntl(_fd,F_SETPIPE_SZ,1024 * 8912);
    }
    if(!_print)
    {
        GLOGI("new pipe: "<< _fn);
        _print=true;
    }
}

/**
 * @brief Fpip::~Fpip
 */
Fpip::~Fpip()
{
    if(_fd)
        ::close(_fd);
    ::unlink(_fn.c_str());
    GLOGI("delete pipe: "<< _fn);
}

/**
 * @brief LogOften
 */
static int LogOften=0;
int Fpip::stream(const uint8_t* buff,size_t maxsz)
{
    if(_fd)
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
                GLOGI ("mplayer " << _fn << " -cache 200 -cache-min 80  # has "<<maxsz<<" bytes ");
            }
            else{
                //std::cout << " pipe full ! " << _fn << " "<<maxsz<<" bytes \n";
            }
        }
        return sent;
    }
    return 0;
}

