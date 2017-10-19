///////////////////////////////////////////////////////////////////////////////////////////////

#ifndef LOGS_H
#define LOGS_H

#include <iostream>
#include <string>
#include <fstream>
#include <sstream>

extern int Debug;

void fileit(std::stringstream& ss,bool fatal=false);

#define GLOGI(x) if(Debug & 0x1) \
do{\
    std::stringstream ss; ss << pthread_self() << "I: "<< x << "\r\n";\
    fileit(ss); \
}while(0);


//-----------------------------------------------------------------------------
// WARNING LOGING
#define GLOGW(x) if(Debug & 0x2)\
do{\
    std::stringstream ss; ss << pthread_self() <<  "W: " << x << "\r\n";\
    fileit(ss); \
}while(0);


//-----------------------------------------------------------------------------
// ERROR LOGING
#define GLOGE(x) \
do{\
    std::stringstream ss; ss << pthread_self() << " E: " << x << "\r\n";\
    fileit(ss); \
}while(0);


//-----------------------------------------------------------------------------
// // TRACE LOGING
#define GLOGD(x) if(Debug & 0x08)\
do{\
    std::stringstream ss; ss << pthread_self() << " D: "<< x << "\r\n";\
    fileit(ss); \
}while(0);

//-----------------------------------------------------------------------------
// OUTPUT LOGING
#define GLOGO(x) if(Debug & 0x10)\
do{\
    std::stringstream ss; ss << pthread_self() << " O: " << x << "\r\n";\
    fileit(ss); \
}while(0);

struct Lio
{
    std::string op;
    Lio(const char* ch);
    ~Lio();
};


#endif // LOGS_H
