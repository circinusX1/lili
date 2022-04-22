#include "acamera.h"
#include "imgsink.h"

acamera::acamera(const std::string& name, const std::string& loc, const Cbdler::Node& n)
{
    _name = name;
    _location = loc;
    _motion   = n["motion"].value();
}

acamera::~acamera()
{
}

void acamera::set_peer(imgsink* peer)
{
    _peer = peer;
}


