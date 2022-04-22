///////////////////////////////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <libgen.h>
#include <unistd.h>
#include <mutex>
#include "logger.h"

int Debug = 0xFFFF;


static std::string LogFile = "/var/log/isapi/srvvideosink.log";
static int    logManage();
static size_t LogFileLastSz;
static size_t MaxLogSize=20000000;
static int    Max_LogFiles=40;
static int    OnceIn100=0;
std::mutex    MuxLog;         //log file mutex
/**
 * @brief fileit,logs LOG in log file
 * @param ss
 * @param fatal
 */
void fileit(std::stringstream& ss,bool fatal)
{
    std::lock_guard<std::mutex> guard(MuxLog);

    bool chown = false;
    FILE *log = ::fopen(LogFile.c_str(),"ab");
    if (!log){
        log = fopen(LogFile.c_str(),"wb");
        chown=true;
    }
    if (!log)
    {
        std::cout << ("Can not open /var/log/isapi/srvvideosink.log for writing.\n");
        std::cout.flush();
        std::cout << "\r\n";
        return;
    }
    if( Debug & 0x10)
        std::cout << ss.str().c_str();
    ::fwrite(ss.str().c_str(),sizeof(char),ss.str().length(),log);
    if(OnceIn100%100==0)
    {
        LogFileLastSz = ::ftell(log);
    }
    ::fclose(log);

    if(chown){
        chmod(LogFile.c_str(),S_IRWXO|S_IRWXG|S_IRWXU);
        system("chown ubuntu:www-data /var/log/isapi/srvvideosink.log");
    }
    if(OnceIn100>10000)
    {
        if(LogFileLastSz > MaxLogSize)
        {
            logManage();
        }
        OnceIn100=0;
    }
    ++OnceIn100;
}


static int logManage()
{
    if(!LogFile.empty())
    {
        char fp[300];
        char fps[300];
        char dir[128] = {0};
        char fname[128] = {0};

        ::strcpy(fp, LogFile.c_str());
        ::strcpy(fname,LogFile.substr(LogFile.find_last_of('/')).c_str());
        ::strcpy(dir,::dirname(fp));

        if(::strchr(fname,'-')!=0)
            *(::strchr(fname,'-'))=0;
        if(LogFileLastSz > MaxLogSize)
        {
            LogFileLastSz = 0;
            for(int k=Max_LogFiles; k>0; k-- )
            {
                ::snprintf(fps,sizeof(fps),"%s%s-%d",dir,fname,k);
                ::snprintf(fp,sizeof(fp),"%s%s-%d",dir,fname,k-1);
                if(k == Max_LogFiles)
                {
                    if(::access(fps,0)==0)
                        ::unlink(fps);
                }
                else
                {
                    if(::access(fp,0)==0)
                    {
                        ::rename(fp,fps);
                    }
                }
            }
            ::rename(LogFile.c_str(),fp);
            ::unlink("/var/log/isapi/srvvidesinkweb.log");

            FILE* pf = ::fopen(LogFile.c_str(),"wb");
            if(pf)
            {
                ::fclose(pf);
                chmod(LogFile.c_str(),S_IRWXO|S_IRWXG|S_IRWXU);
                system("chown popina:www-data /var/log/isapi/srvvidesink.log");
            }
        }
    }
    return 0;
}



Lio::Lio(const char* ch):op(ch)
{
    GLOGI("K: " << ch << "{");
}

Lio::~Lio()
{
    GLOGI("K: " << op << "}");
}
