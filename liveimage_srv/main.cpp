///////////////////////////////////////////////////////////////////////////////////////////////

#include "os.h"
#include "sock.h"
#include "rawsock.h"
#include "tcpsrv.h"
#include "pool.h"
#include "cliqueue.h"
#include "oneproc.h"
#include "sheller.h"
#include "logger.h"
#include "config.h"
#include "encrypter.h"

/**
    ulimit -S -c unlimited /home/ubuntu/na3_work/cameraisapi/srvvideosink/srvvideosink
    sysctl -w kernel.core_pattern=/home/ubuntu/isapi_crash/core-%e-%s-%u-%g-%p-%t


    gdb -c dump-file

    (gdb)file /path/to/executable
    (gdb) bt

 */
char        __files_recs[256];
bool        __alive = true;
char        __server_key[32] = {0};
Encryptor*  pENC;
Cbdler*     pCONF;


std::string empty_string;

/**
 * @brief ControlC
 * @param i
 */
void ControlC (int i)
{
    (void)i;
    __alive = false;
    printf("Exiting...\n");
}

/**
 * @brief Sigs
 */
static void Sigs()
{
    signal(SIGINT, ControlC);
    signal(SIGABRT,ControlC);
    signal(SIGTERM,ControlC);
    signal(SIGKILL,ControlC);

    sigset_t mask;
    sigemptyset(&mask);
    sigaddset(&mask,SIGINT);
    sigaddset(&mask,SIGPIPE);
    pthread_sigmask(SIG_UNBLOCK, &mask, NULL);
}

static void loop_callback();
/**
 * @brief main
 * @param argc
 * @param argv
 * @return
 */

Wd    Wdog;



int main(int argc,char *argv[])
{
    OneProc ps (12841);         // singleton,limit this instance to 1

    if(!ps())                   // second instance
    {
        GLOGI("already running");
        return -1;
    }

    std::string config = "./liveimage_srv.conf";
    Cbdler      cdb;

    try{
        cdb.parse(config.c_str());
    }catch(int ex)
    {
        exit (ex);
    }

    pCONF = &cdb;
    pENC = new Encryptor;

    ::strncpy(__files_recs,cdb["records"].value().c_str(),sizeof(__files_recs));
    if(::access(__files_recs,0)!=0)
    {
        std::cout << "Canot find: " << __files_recs <<". Please create the folder\n";
        exit(-2);
    }

    CliQ      q;
    Sheller   tm;
    Pool      p(q,tm);
    TcpSrv    l(p,q,&tm);

    std::cout << "starting " << "\r\n";
    std::string dbg = cdb["verbose"].value();
    if(dbg.find('I')!=std::string::npos)Debug |= 0x1;
    if(dbg.find('W')!=std::string::npos)Debug |= 0x2;
    if(dbg.find('D')!=std::string::npos)Debug |= 0x4;
    if(dbg.find('O')!=std::string::npos)Debug |= 0x8;
    std::cout << "DBG = " << std::hex << Debug << "\n";

    ::strncpy(__server_key,cdb["password"].value().c_str(),sizeof(__server_key)-1);

    // to see cameras http://server/?__server_key
    if(::strlen(__server_key)<6)
    {
        std::cout << "Server key to short. Minimum 7 \n";
        exit(-2);
    }
    int cliport = ::atoi(cdb["cliport"].value().c_str());
    int camport = ::atoi(cdb["camport"].value().c_str());


    tm.bind(&p);
    GLOGI(strweb_time());
    Sigs();
#ifndef DEBUG
    Wdog.start_thread();
#endif

    p.start_thread();
    l.spin(camport,cliport,loop_callback);
    p.stop_thread();
    delete pENC;
}

static void loop_callback()
{
    ;
}

