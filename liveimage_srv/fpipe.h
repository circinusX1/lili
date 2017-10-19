///////////////////////////////////////////////////////////////////////////////////////////////
#ifndef FPIPE_H
#define FPIPE_H

#include <string>
#include "os.h"

class Fpip
{
public:
    Fpip(std::string file);
    ~Fpip();

    int stream(const uint8_t* buff,size_t maxsz);
    bool ok()const{return _fd>0;}

private:
    std::string _fn;
    int         _fd;
    bool        _print;
};

#endif // FPIPE_H
