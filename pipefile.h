#ifndef PIPEFILE_H
#define PIPEFILE_H



#include <string>
#include "os.h"

class pipiefile
{
public:
    pipiefile(std::string file);
    ~pipiefile();

    int stream(const uint8_t* buff,size_t maxsz);
    bool ok()const{return _fd>0;}

private:
    std::string _fn;
    int         _fd;
    bool        _print;
};

#endif // PIPEFILE_H
