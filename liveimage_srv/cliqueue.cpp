///////////////////////////////////////////////////////////////////////////////////////////////
#include "cliqueue.h"
#include "logger.h"
/**
 * @brief CliQ::CliQ
 */
CliQ::CliQ()
{
}

CliQ::~CliQ()
{
    AutoLock a(&_m);
    while(_q.size()) {
        delete _q.front();
        _q.pop_front();
    }
}

/**
 * @brief CliQ::push
 * @param ps
 * @return
 */
bool CliQ::push(RawSock* ps)
{
    AutoLock a(&_m);
    GLOGW("PUSHED ");
    _q.push_back(ps);
    return true;
}

/**
 * @brief CliQ::pop
 * @return
 */
RawSock* CliQ::pop()
{
    AutoLock a(&_m);
    if(_q.size()) {
        RawSock *ppc = _q.front();
        _q.pop_front();
        GLOGW("POPPED ");
        return ppc;
    }
    return nullptr;
}
