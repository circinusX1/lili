#ifndef IMGSINK_H
#define IMGSINK_H

#include <string>
#include "lilitypes.h"
#include "cbconf.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class imgsink
{
public:

    imgsink(const std::string& name);
    virtual ~imgsink();
    virtual bool init(const dims_t&)=0;
    virtual bool stream(const uint8_t* pb, size_t len, const dims_t& imgsz,
                        const std::string& name, const event_t& event, EIMG_FMT eift, time_t now)=0;
    virtual bool spin() = 0;
    const std::string& name()const { return _name; }
    virtual const std::string cache()=0;

protected:
    std::string _name;
};

#endif // IMGSINK_H
