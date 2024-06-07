///////////////////////////////////////////////////////////////////////////////////////////////
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include "main.h"
#include "pool.h"
#include "sheller.h"

Sheller::Sheller():_pool(nullptr)
{
}

/**
 * @brief Sheller::~Sheller
 */
Sheller::~Sheller()
{
    while(_procs.size())
    {
        _term_prc(*_procs.begin());
    }
}

/**
 * @brief Sheller::run
 * @param runop
 * @param mac
 * @param eva
 * @return
 */
size_t Sheller::run(const std::string& pcsname,
                    const std::string& runop,
                    const std::string& mac,
                    const std::string& eva)
{
    AutoLock a(&_m);
    GLOGI( "TM: Command to run: " << runop << "," << mac <<","<< eva );
    _commands.push_back(PrcCmds {true,runop,mac,eva,pcsname});
    return _commands.size();
}

/**
 * @brief Sheller::stop
 * @param runop
 * @param mac
 * @param eva
 * @return
 */
size_t Sheller::stop(const std::string& runop,
                     const std::string& mac,
                     const std::string& eva,
                     const std::string& pcsname)
{
#if 0
    AutoLock a(&_m);
    GLOGI( "TM: Command to stop: " << runop << "," << mac <<","<< eva );

    _commands.push_back(PrcCmds {false,runop,mac,eva,pcsname});
    return _commands.size();
#else
    return 0; //taken care by monitor loop
#endif
}

/**
 * @brief spwan_prc
 * @param program
 * @param args
 * @return
 */
static pid_t spwan_prc(const char* program,char* args[])
{
    pid_t child_pid = fork();
    if (child_pid != 0)
    {
        return child_pid;
    }
    else
    {
        GLOGI( "TM: child: spawing: "<<program <<" " <<args[1]<<" " << args[2]);
        ::execv(program,args);
        GLOGI("EXITING: cannt spawn process. " <<  strerror(errno) );
        exit(EXIT_FAILURE);
    }
    return 0;
}

/**
 * @brief Sheller::thread_main
 */
void Sheller::thread_main()
{
    PrcCmds  cmd;
    //    int     cams,clis;
    assert(0);  // stopped june 4 2022

    while(__alive && _bstop==0)
    {
        ::sleep(1);
        do{
            AutoLock a(&_m);
            if(_commands.size())
            {
                cmd = _commands.front();
                _commands.pop_front();
            }
        }while(0);
        if(!cmd.mac.empty())
        {
            ::sleep(1);
            std::string proc ;
            proc += cmd.runop+" ";
            proc += cmd.mac+" ";
            proc += cmd.eva;

            if(cmd.start==true)
            {
                Procc fnd;
                for(const auto& a : _procs)
                {
                    if(a.exe==proc)
                    {
                        fnd=a;
                        break;
                    }
                }
                if(fnd.pid)
                {
                    GLOGE("TM: This should not happen");
                    _term_prc(fnd);
                }
                _exec(cmd,proc);
            }
            cmd.mac.clear();
        }
        _monitor();
    }

}

/**
 * @brief Sheller::_terminate
 * @param fnd
 * @param proc
 */
void Sheller::_term_prc(const Procc& p)
{
    GLOGI( "TM:{ " << __FUNCTION__ );
    std::string proc;
    std::string cmd = _kill_by_name(p.pcsname,p.pid);

    ::msleep(0xFF);
    if(_popen(cmd.c_str(),proc))
    {
        GLOGE ("TM: Error. Cannot kill." << cmd << "Killing all... No recover " << cmd );
    }
    else
    {
        ::unlink(p.exe.c_str());
        _procs.erase(p);
    }

    GLOGI("TM: RUNNING AFTER KILL");
    for(const auto &a :_procsnames)
    {
        GLOGI(a.first << "," << a.second);
    }
    GLOGI( "TM: there are: " << _procs.size() << " running");
    GLOGI( "TM:} " << __FUNCTION__ );
}

void Sheller::_exec(PrcCmds& cmd,const std::string& proc)
{


    char*  argv[] = {(char*)proc.c_str(),
                     (char*)cmd.mac.c_str(),
                     (char*)cmd.eva.c_str(),(char*)nullptr};
    int status = ::spwan_prc( proc.c_str(),argv);
    if(status)
    {
        Procc prc;
        prc.pid = status;
        prc.exe = proc;
        prc.pcsname = cmd.pcsname;
        prc.maccam  = cmd.mac;
        prc.cmdline += cmd.runop+cmd.mac+cmd.eva; //
        _procs.insert(prc);
        if(_procsnames.find(cmd.runop)==_procsnames.end())
            _procsnames[cmd.runop]=1;
        else
            _procsnames[cmd.runop]++;
        GLOGI( "TM: STARTED: " << proc << " PID=" << status);
    }
    else
    {
        GLOGE( "TM: Error"  << strerror(errno)<< ". Cannot start " << proc);
    }
    GLOGI("TM: RUNNING AFTER SPAWN");
    for(const auto &a :_procsnames)
    {
        GLOGI(a.first << ","<<a.second);
    }

}

/**
 * @param script
 * @return
 */

/**
 * @brief Sheller::update
 * @param cams
 * @param clis
 */
void Sheller::update(int cams,int clis)
{
    AutoLock a(&_m);
    _cams=cams;
    _clis=clis;
}

/**
 * @brief Sheller::_monitor
 */
void Sheller::_monitor()
{
    std::string cmd ;
AGAIN:
    ::msleep(512);
    for(const auto& a : _procs)
    {
        if(!_pool->has(a.pcsname))
        {
            _term_prc(a); // this erases from _procs
            cmd =  "kill -9 ";
            cmd += std::to_string(a.pid);
            ::system(cmd.c_str());
            GLOGI(" systeming: " << cmd);
            goto AGAIN;
        }
    }

    if(_procs.size() == 0 )
    {
        if(!_killed.empty())
        {
            std::string cmd = "pkill ";
            cmd += _killed + "*";
            ::system(cmd.c_str());
            GLOGI(" Cleaning up: " << cmd );
            _killed.clear();
        }
        else if(_procsnames.size())
        {
            std::string  proc;
            std::string  comand;
AGAIN2:
            for(auto &a : _procsnames)
            {
                GLOGI("MON: Killing remaining processes.");
                std::string  cmd = " /bin/ps ax | grep ";
                cmd += a.first;
                cmd += " | grep -v grep";
                proc.clear();
                if(_popen(cmd.c_str(),proc))
                {
                    pid_t pid = ::atoi(proc.c_str());
                    comand = "pkill -9 ";
                    comand += a.first;
                    ::system(a.first.c_str());
                    a.second=0;
                    GLOGI(comand);

                    if(pid !=0)
                    {
                        _kill_by_name(a.first);
                    }
                }
                else
                {
                    GLOGI("Successfuly kiled " << a.first);
                    _procsnames.erase(a.first);
                    goto AGAIN2;
                }
            }
        }
    }
}

/**
*/
bool Sheller::_popen(std::string command,std::string& ret)
{
    FILE* pipe = popen(command.c_str(),"r");
    if ( pipe==NULL )
    {
        GLOGE("TM: Cannot open command shell: " << command);
        return false;
    }

    char buffer[256]={0};
    ret.clear();
    while( !feof(pipe) )
    {
        if( fgets(buffer,255,pipe) != NULL )
        {
            ret += buffer;
        }
    }
    pclose(pipe);
    GLOGI("TM: Command: " << command << " returned:" << ret);
    return ret.length()>4;
}

bool  Sheller::is_runing(const std::string& mac)
{
    AutoLock a(&_m);
    if(_procs.size())
    {
        GLOGI("searching in " << _procs.size() << " processes: MAC:" << mac);
        for(const auto& a : _procs)
        {
            if(a.maccam==mac)
            {
                GLOGI("TM: Found process: " << a.cmdline);
                return true;
            }
        }
    }
    return false;
}

std::string Sheller::_readfile(const std::string& file)
{
    std::ifstream expFile;
    std::string   out;

    try{
        expFile.open( file,std::ios::in);
        if(expFile.is_open())
        {
            expFile >> out;
            expFile.close();
            if(!out.empty())
            {
                size_t eos = out.find_first_of('\n');
                if(eos != std::string::npos)
                {
                    out = out.substr(0,eos);
                }
            }
        }
        else
        {
            GLOGE (" Cannot read: " << file );
        }
    }
    catch (std::exception& e)
    {
        GLOGE (" std exception "<< e.what() );
    }
    return out;
}

std::string Sheller::_kill_by_name(const std::string& pcsname,pid_t pid)
{

    if(pid)
    {
        pid_t wpid;
        int timeout  = 10;
        int status;
        for(int i=0;i<4;i++)
        {
            GLOGI( "TM: *** *** Killing: PID"  << pid <<","<< pcsname);
            ::kill(pid,SIGKILL);
            do {
                wpid = waitpid(pid,&status,WUNTRACED | WCONTINUED);

                if (wpid == -1)
                {
                    GLOGE("CANNOT KILL THIS PROCESS:" << strerror(errno));
                    break;
                }
                ::sleep(1);
                GLOGI(" waiting for pid " << pid << ",to:" << timeout);
            } while (!WIFEXITED(status) && !WIFSIGNALED(status) && --timeout>0);
        }
    }

    std::string  cmd = " /bin/ps ax | grep ";
    cmd += pcsname;
    cmd += " | grep -v grep";
    std::string  proc;

    for(int i=0;i<4;i++)
    {
        if(_popen(cmd.c_str(),proc))
        {
            pid = ::atol(proc.c_str());
            if(pid)
            {
                pid_t wpid;
                int timeout  = 10;
                int status;

                GLOGI( "TM: *** *** Killing: " << proc  << " *** PID:" << pid <<","<< pcsname);
                ::kill(pid,SIGKILL);
                do {
                    wpid = waitpid(pid,&status,WUNTRACED | WCONTINUED);
                    if (wpid == -1)
                    {
                        GLOGE("CANNOT KILL THIS PROCESS:" << strerror(errno));
                        break;
                    }
                    ::sleep(1);
                    GLOGI(" waiting for pid " << pid << ",to:" << timeout);
                } while (!WIFEXITED(status) && !WIFSIGNALED(status) && --timeout>0);
                GLOGI("Exit status:" << pid << " = " << status);
                ::msleep(128);
            }
            else
            {
                GLOGE(" Cannot get PID for:" << proc);
                break;
            }
        }
        else
            break;
    }
    return cmd;
}
