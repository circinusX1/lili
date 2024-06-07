///////////////////////////////////////////////////////////////////////////////////////////////
#include "main.h"
#include "pool.h"
#include "tcpcamcli.h"
#include "rawsock.h"
#include "cliqueue.h"
#include "sheller.h"


Pool::~Pool()
{
    closeall();
    signal_to_stop();
}

/**
 * called from thread
 * @brief Pool::destroy
 */
void     Pool::closeall()
{
    //_tm.update(0,0);
    AutoLock a(&_m);

    for(auto& a : _pool)
    {
        DELETE_PTR(a.second->_cam);
        Pair* pr = a.second;
        if(pr->_clis.size())
        {
            for(auto& csc: pr->_clis)
            {
                DELETE_PTR(csc);
            }
            pr->_clis.clear();
        }
        delete pr;
    }
    _pool.clear();
}

/**
 * @brief Pool::has
 * @param name
 * @return
 */
TcpCamCli*   Pool::has(const std::string& name)
{
    AutoLock a(&_m);

    if(_pool.size()){
        kconiterator it = _pool.find(name);

        return it==_pool.end() ?
                    nullptr :
                    (*it).second->_cam;
    }
    return nullptr;
}


/**
 * @brief Pool::has
 * @param sin
 * @return
 */
bool    Pool::has(const RawSock& sin)const
{
    AutoLock a(&_m);
    if(_pool.size()==0){
        return false;
    }
    for(const auto& a : _pool)
    {
        Pair* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            if(css->Rsin().c_str() == sin.Rsin().c_str())
            {
                return true;
            }
        }
    }
    return false;
}



/**
 * @brief Pool::_has
 * @param name
 * @return
 */
Pool::Pair*  Pool::_has(const std::string& name)
{
    if(_pool.size()==0){
        return nullptr;
    }
    AutoLock a(&_m);
    coniterator it = _pool.find(name);
    return it==_pool.end() ? nullptr : (*it).second;
}

/**
 * @brief Pool::getChannels
 * @param webreq
 * @return
 */
std::string Pool::get_cams(const std::string& host,bool webreq)const
{
    AutoLock a(&_m);

    std::string hret = "<!DOCTYPE html>\r\n<html lang='en'>\r\n";
    hret += "<head><meta http-equiv='refresh' content='5'></head>\r\n";
    hret += "<h2>STREAMS</h2><br>\r\n";

    std::map<std::string, std::string>  lis;

        if(_times.size())
        {
            for(const auto& a : _times)
            {
                std::string ret = "<li> Camera: <a href='http://";
                ret+=host;
                ret+="?";
                ret+=a.first;
                ret+="'><b>";
                ret+="<i>"; ret+=a.first; ret += "</i> is active ";
                ret+="</b></a>";
                lis[a.first] = ret;
            }
        }

    if(_pool.size())
    {
        for(const auto& a : _pool)
        {
            const TcpCamCli* pc = a.second->_cam;
            lis[pc->name()] += " and is streaming.\r\n";
        }
    }
    for(const auto& a: lis){
        hret += a.second;
    }


    hret+="</html>\r\n";
    return hret;
}

/**
 * @brief Pool::getChannelHeader
 * @param channel
 * @return   [FIRST=CHANNEL-CAM,SECOND CLI-CONN]
 */
const LiFrmHdr* Pool::getChannelHeader(const char* channel)const
{
    AutoLock a(&_m);
    if(_pool.size()==0){
        return nullptr;
    }

    for(const auto& a : _pool)
    {
        if(a.second->_cam->name()==channel)
            return a.second->_cam->header();
    }
    return nullptr;
}

/**
 * @brief Pool::thread_main
 */
void Pool::thread_main()
{
    fd_set  fdr;
    fd_set  fdw;
    fd_set  fdx;
    int     ndfs;
    bool    dirty=false;

    Wdog.reg();
    //_tm.start_thread();
    while(!is_stopped() && __alive)
    {
        do{
            AutoLock a(&_m);
            _checkNew(dirty);
        }while(0);

        ndfs = _fdSet(fdr,fdw,fdx);
        if(ndfs)
        {
            dirty = _fdCheck(fdr,fdw,fdx,ndfs);
        }
        ::usleep(4000);
        Wdog.pet();
    }
    //_tm.signal_to_stop();
    closeall();
}

/**
 * @brief Pool::signal_to_stop
 */
void Pool::signal_to_stop()
{

    for(const auto& a : _pool)
    {
        if(a.second->_cam!=nullptr)
        {
            _kill_conn(a.second->_cam);
        }
        const Pair* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            _kill_conn(css);
        }
    }
    OsThread::signal_to_stop();
}

/**
 * @brief Pool::_fdSet
 * @param fdr
 * @return
 */
int Pool::_fdSet(fd_set& fdr,fd_set& fdw, fd_set& fdx)
{
    int ndfs = 0;
    FD_ZERO(&fdr);
    FD_ZERO(&fdw);
    FD_ZERO(&fdx);

    AutoLock a(&_m);

    for(const auto& a : _pool)
    {
        FD_SET(a.second->_cam->socket(),&fdr);
        FD_SET(a.second->_cam->socket(),&fdw);
        FD_SET(a.second->_cam->socket(),&fdx);
        ndfs = std::max(ndfs,a.second->_cam->socket());
        const Pair* pr = a.second;
        for(const auto& css : pr->_clis)
        {
            if(css && css->isopen())
            {
                FD_SET(css->socket(),&fdr);
                // FD_SET(css->socket(),&fdw);
                // FD_SET(css->socket(),&fdx);
                ndfs = std::max(ndfs,css->socket());
            }
        }
    }
    return ndfs ? ndfs+1 : 0;
}

/**
 * @brief Pool::_fdCheck
 * @param fdr
 * @param ndfs
 * @return
 */

bool Pool::_fdCheck(fd_set& fdr,fd_set& fdw,fd_set& fdx,int ndfs)
{
    bool    dirty = false;
    timeval tv {0,2000};
    int     sel =::select(ndfs,&fdr,&fdw,&fdx,&tv);
    static  time_t  nowt = 0;

    if(sel==-1){
        std::cerr << " socket select error critical \n";
        __alive=false;
        return false;    //dirty
    }
    if(sel==0){ return true; }

    for(auto& a : _pool)
    {
        if(!a.second->_cam->isopen()) {
            continue;
        }
        if(FD_ISSET(a.second->_cam->socket(),&fdx))
        {
            _kill_conn(a.second->_cam);
            continue;
        }
        if(FD_ISSET(a.second->_cam->socket(),&fdw))
        {
            a.second->_cam->can_send();
        }
        if(FD_ISSET(a.second->_cam->socket(),&fdr))
        {
            try{
                a.second->_cam->transfer(a.second->_clis);
                if(SECS()-nowt>CHECK_CLIENTS_TOUT)
                {
                    _check_activity(a.second->_cam,a.second->_clis.size());
                    nowt = SECS();
                }
            }
            catch(RawSock::STYPE s)
            {
                dirty = true;
            }
        }

    }

    return dirty;
}

void Pool::_check_activity(TcpCamCli* cam,int clients_count)
{
    this->record_cam(cam->name());
    GLOGD(__FUNCTION__);
    if(clients_count){
        GLOGD(__FUNCTION__);
        this->record_cli(cam->name());
    }else{
        if(this->client_was_here(cam->name())==false)
        {
            time_t lt = SECS() - cam->last_time();
            if(lt > MOTION_REC_TRAIL)
            {
                _kill_conn(cam, true);
            }
        }
    }
}


/**
 * @brief Pool::_checkNew
 * @param dirty
 */
void Pool::_checkNew(bool dirty)
{
    RawSock*  pcs = _q.pop();
    if(pcs)
    {
        pcs->bind(&_tm);

        Pool::Pair* pexistentCam = _has(pcs->name());     /* a camera conn */

        if(pexistentCam==nullptr)
        {
            if(pcs->type()==RawSock::CAM) // is camera socket
            {
                _add_cam(pcs);
                return;
            }
        }
        else
        {
            if(pcs->type()==RawSock::CLIENT||
                    pcs->type()==RawSock::CLIJPEG||
                    pcs->type()==RawSock::CLIRTSP) // check if is a Client socket
            {
                _add_client(pexistentCam,pcs);
                return;
            }
        }
        delete pcs;
    }
    if(dirty)
    {
        _clean();
    }
}


void Pool::killSocket(const Request& req)
{
//    AutoLock a(&_m);
}

/**
 * @brief Pool::_clean
 */
void Pool::_clean()
{
AGAIN:
    coniterator it = _pool.begin();
    for(;it!=_pool.end();++it)
    {
        Pair* pr =   (*it).second;
        //
        // if camera got away
        //
        if(pr->_cam->isopen()==false)
        {
            GLOGD( " STREAM GONE. DROPPING CLIENTS TOO");
            for(auto& client : pr->_clis)
            {
                _del_client(pr,client);
                _kill_conn(client);
                DELETE_PTR(client);
            }
            pr->_clis.clear();
            _del_camera(pr->_cam);
            DELETE_PTR (pr->_cam);
            _pool.erase(it);
            DELETE_PTR(pr);

            goto AGAIN;
        }
        //
        // if a client got away
        //
        if(pr->_clis.size())
        {
DELVCT:
            std::vector<RawSock*>::iterator it = pr->_clis.begin();
            for(;it!=pr->_clis.end();it++)
            {
                if((*it)->isopen()==false)
                {
                    if(pr->_cam)
                    {
                        pr->_cam->bind((TcpWebSock*)*it,false);
                    }
                    _del_client(pr,*it);
                    _kill_conn((*it));
                    DELETE_PTR(*it);
                    pr->_clis.erase(it);
                    GLOGD( "        DROPPING CLIENT");
                    goto DELVCT;
                }
            }
        }
    }
}

/**
 * @brief Pool::_del_client
 * @param cs
 */
void Pool::_del_client(Pair* pr,RawSock* cs)
{
    AutoLock a(&_m);
    GLOGI("Client disconnected");
    if(pr->_cam && pr->_cam->isopen())
    {
        pr->_cam->sendall("client_off",7);
    }
}

/**
 * @brief Pool::_add_client
 * @param pcamerain
 * @param pcs
 */
void Pool::_add_client(Pair* pcamerain,RawSock* pcs)
{
    pcamerain->_clis.push_back(pcs);
    pcamerain->_cam->bind((TcpWebSock*)pcs,true);
    //pcamerain->_cam->sendall("client_on",6);
}

/**
*/
bool Pool::has_cam(const std::string& pcsname)
{
    AutoLock a(&_m);
    if(_pool.size()==0)return false;

    return _pool.find(pcsname)!=_pool.end();
}

/**
 * @brief Pool::_add_cam
 * @param pcs
 */
void Pool::_add_cam(RawSock* pcs)
{
    Pair* p  = new Pair();

    p->_cam = (TcpCamCli*)(pcs);
    p->_cam->bind(nullptr,false);
    p->_clis.clear();


    if(_pool.find(pcs->name())!=_pool.end())
    {
        _kill_conn(_pool[pcs->name()]->_cam);
        delete _pool[pcs->name()];
    }
    AutoLock a(&_m);

    GLOGI("ADDINF KEEP CAM " << pcs->name());

    _pool[pcs->name()] = p;
    size_t seq = _seqs[pcs->name()];
    GLOGD(" reindexing file saving at " << seq);
    pcs->seq(seq);
}

/**
 * @brief Pool::kill_cam
 * @param cs
 */
void Pool::kill_cam(const char* cs)
{
    AutoLock a(&_m);
    if(_pool.find(cs)!=_pool.end())
    {
        if(_pool[cs])
        {
            Pair* pp = _pool[cs];
            pp->_cam->set_letal();
            _kill_conn(pp->_cam);
        }
    }
}

/**
 * @brief Pool::_del_camera
 * @param cs
 */
void Pool::_del_camera(TcpCamCli* cs)
{

}

void Pool::_wipe_activity(const char* cam)
{
    if(_times.find(cam)!=_times.end())
    {
        _times.erase(cam);
    }
}

void Pool::record_cli(const std::string& camname)
{
    time_t now = SECS();
    _times[camname].cli = now;
}

void Pool::record_cam(const std::string& camname)
{
    time_t now = SECS();
    GLOGI("RECORD CAM = " << camname);
    _times[camname].cam = now;
}

bool Pool::client_was_here(const std::string& cam)
{
    if(_times.find(cam)!=_times.end())
    {
        time_t d = SECS() - _times[cam].cli;
        if(d < KILL_TIME)
        {
            return true;
        }
        return false;
    }
    return false;
}

bool Pool::camera_kicking(const std::string& cam)
{
    std::map<std::string,Times>::iterator fi = _times.find(cam);
    if(fi !=_times.end())
    {
        time_t d = SECS();
        Times& tt = fi->second;
        return d - tt.cam < KILL_TIME;
    }
    return false;
}


void Pool::kill_times()
{
    time_t now = SECS();
AGAIN:

    for(const auto &a : _times)
    {
        const Times& pt = a.second;
        if(now - pt.cam > KILL_TIME)
        {
            GLOGI("ERASING  CAM CLI WAITIME " << a.first << "," << now);
            _times.erase(a.first);
            goto AGAIN;
        }
    }
}

void Pool::_kill_conn(RawSock* ps, bool trw)
{
    ps->destroy();
    _seqs[ps->name()] = ps->seq();
    GLOGD(" KILLING CAM CONN  file saving seq at " << ps->seq());

    if(trw)
        throw ps->type();
}
