///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef CLIQUEUE_H
#define CLIQUEUE_H


#include <deque>
#include "rawsock.h"

class Pool;
class RawSock;
class CliQ
{
public:
    CliQ();
    virtual ~CliQ();
    bool push(RawSock*);
    RawSock* pop();

private:
    deque<RawSock*>   _q;
    umutex      _m;
};

#endif // CLIQUEUE_H
