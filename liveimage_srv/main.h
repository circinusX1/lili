#ifndef MM_H
#define MM_H

#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>
#include "logfile.h"
#include "os.h"


extern bool __alive;
extern std::string empty_string;
extern int CliPort;
#define EOS '\0'
#define     WD_TIME     22

class Cbdler;
extern Cbdler*      pCONF;

inline void sreplace(std::string& str,char seq,char rep)
{
    while(str.find(seq) != std::string::npos)
    {
        str.replace(str.find(seq),1,1,rep);
    }
}

inline void sreplaces(std::string& subject,
                        const std::string& search,
                        const std::string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search,pos)) != std::string::npos)
    {
         subject.replace(pos,search.length(),replace);
         pos += replace.length();
    }
}


inline void printbin(const uint8_t* str,size_t len)
{
    for(size_t i=0;i<len;i++)
    {
        ::printf("%02X:",int(str[i]));
    }
    printf("\n");

}

inline size_t split(const std::string& str,
                    char delim,std::vector<std::string>& a,
                    char rm=0)
{
    if(str.empty())
        return 0;

    std:: string token;
    size_t prev = 0,pos = 0;
    size_t sl = str.length();
    do
    {
        pos = str.find(delim,prev);
        if (pos == std::string::npos)
            pos=sl;
        token = str.substr(prev,pos-prev);
        if (!token.empty())
        {
            if(rm)
            {
                size_t rmp = token.find(rm);
                if(rmp != std::string::npos)
                {
                    token=token.substr(0,rmp);
                }
            }
            a.push_back(token);
        }
        prev = pos + 1;
    }while (pos < sl && prev < sl);
    return a.size();
}

struct UrlInfo
{
    UrlInfo(){port=80;}
    char  host[128];
    char  path[128];
    char  username[32];
    char  password[32];
    int   port;
};

inline int ParseUrl(const char *u,UrlInfo *urlinfo)
{
    char surl[256];
    char* url=surl;

    ::strcpy(url,u);
    char *s = strstr(url,"://");
    int ret;

    if (s) url = s + strlen("://");
    memcpy(urlinfo->path,(void *)"/\0",2);

    if (strchr(url,'@') != NULL)
    {
        ret = sscanf(url,"%[^:]:%[^@]",urlinfo->username,urlinfo->password);
        if (ret < 2) return -1;
        url = strchr(url,'@') + 1;
    }
    else
    {
        urlinfo->username[0] = '\0';
        urlinfo->password[0] = '\0';
    }

    if (strchr(url,':') != NULL)
    {
        ret = sscanf(url,"%[^:]:%hu/%s",urlinfo->host,
                     (short unsigned int*)&urlinfo->port,urlinfo->path+1);
        if (urlinfo->port < 1) return -1;
        ret -= 1;
    }
    else
    {
        urlinfo->port = 80;
        ret = sscanf(url,"%[^/]/%s",urlinfo->host,urlinfo->path+1);
    }
    if (ret < 1) return -1;

    return 0;
}

class Wd : public OsThread
{
public:
    virtual void thread_main()
    {
        while(__alive)
        {
            ::sleep(5);
            do{
                AutoLock a(&_m);

                time_t now = time(0);
                for(const auto& a : _times)
                {
                    if(now - a.second > WD_TIME){
                        GLOGE("THREAD:" << a.first << " is locked");
                        goto DONE;
                    }
                }
            }while(0);
        }
DONE:
        ::exit(10);
    }

    void reg()
    {
        AutoLock a(&_m);
        _times[pthread_self()]=time(0);
    }
    void pet()
    {
        AutoLock a(&_m);
         _times[pthread_self()]=time(0);
    }

private:
    std::map<pthread_t, time_t> _times;
    umutex                      _m;

} ;


#define NPOS std::string::npos
#define DELETE_PTR(p)   if(p) delete p; p=nullptr
extern char __files_recs[256];// "/var/www/html/liveimage/pipes/VCT"
extern char __server_key[32];
extern uint32_t  __camms_key;
extern Wd    Wdog;
#define CFG (*pCONF)

#endif // MM_H
