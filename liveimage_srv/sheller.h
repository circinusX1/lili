///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef SHELLER_H
#define SHELLER_H


#include <deque>
#include <set>
#include <map>
#include "os.h"

/**
 * @brief The Sheller class
 */
class Pool;
class Sheller : public OsThread
{
public:
    Sheller();
    virtual ~Sheller();
    size_t run(const std::string& pcsname,
               const std::string& runop,
               const std::string& mac,
               const std::string& eva);
    size_t stop(const std::string& runop,
                const std::string& mac,
                const std::string& eva,
                const std::string& pcsname);
    void thread_main();
    void update(int cams,int clis);
    void bind(Pool* p){_pool=p;}
    bool  is_runing(const std::string& mac);
private:
    struct PrcCmds{
        bool        start;
        std::string runop;
        std::string mac;
        std::string eva;
        std::string pcsname;
    };
    struct Procc{
        Procc():pid(0){}
        pid_t pid;
        std::string exe;
        std::string pcsname;
        std::string cmdline;
        std::string maccam;
        friend bool operator<(const Procc& a,const Procc& b){return a.pid<b.pid;}
    };

    void _terminate(const Procc& fnd,const std::string& proc);
    void _exec(PrcCmds& maceva,const std::string& exename);
    void _term_prc(const Procc& p);
    void _monitor();
    bool _popen(std::string command,std::string& ret);
    bool _rux_scr(const PrcCmds& meva,const std::string& script);
    std::string _readfile(const std::string& file);
    std::string  _kill_by_name(const std::string& pcsname,pid_t p=0);
private:
    std::deque<PrcCmds>          _commands;
    umutex                      _m;
    std::set<Procc>             _procs;
    std::map<std::string,int>   _procsnames;
    std::string                 _killed;
    int                         _cams;
    int                         _clis;
    Pool*                       _pool;
};

#endif // SHELLER_H
