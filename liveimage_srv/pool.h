///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef POOL_H
#define POOL_H


#include <string>
#include <map>
#include "rawsock.h"
#include "logfile.h"
#include "os.h"

#ifdef DEBUG
#define KILL_TIME           40
#else
#define KILL_TIME           25
#endif
#define CHECK_CLIENTS_TOUT  22
#define CHECK_CLIENT_COUNT  51
#define MOTION_REC_TRAIL    6



struct Request;
class CliQ;
class TcpCamCli;
class RawSock;
class Sheller;
struct LiFrmHdr;

class Pool : public OsThread
{
public:
    struct Pair{
        TcpCamCli* _cam;
        std::vector<RawSock*> _clis;
    };
    struct Times{
        time_t cam;
        time_t cli;
    };

    Pool(CliQ& q,Sheller& tm):_q(q),_tm(tm){
    }
    ~Pool();
    TcpCamCli*    has(const std::string& name);
    bool        has(const RawSock& sin)const;
    std::string get_cams(const std::string& host,bool webreq)const;
    const  LiFrmHdr* getChannelHeader(const char* channel)const;
    void closeall();
    void thread_main();
    void signal_to_stop();
    void killSocket(const Request& req);
    bool has_cam(const std::string& pcsname);
    void record_cli(const std::string& for_cam);
    void record_cam(const std::string& cam);
    void kill_times();
    bool client_was_here(const std::string& cam);
    void kill_cam(const char* cs);
    bool camera_kicking(const std::string& cam);

private:
    int _fdSet(fd_set& fdr,fd_set& fdw,fd_set& fdx);
    bool _fdCheck(fd_set& fdr,fd_set& fdw,fd_set& fdx,int ndfs);
    void _checkNew(bool);
    void _clean();
    Pair*  _has(const std::string& name);
    void _add_cam(RawSock* pcs);
    void _add_client(Pair*,RawSock* pcs);
    void _del_client(Pair*,RawSock* cs);
    void _del_camera(TcpCamCli* cs);
    void _check_activity(TcpCamCli* cam,int clients_count);
    void _wipe_activity(const char*);
    void _kill_conn(RawSock* ps, bool trw=false);

private:

    typedef std::map<std::string,Pair*>::iterator coniterator;
    typedef std::map<std::string,Pair*>::const_iterator kconiterator;
    CliQ&                           _q;
    std::map<std::string,Pair*>     _pool;
    std::map<std::string,Times>     _times;
    std::map<std::string,size_t>    _seqs;
    umutex                          _m;
    Sheller&                        _tm;
};

extern RawSock * CS;

#endif // POOL_H
