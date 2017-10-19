#ifndef OUTSTRMFMT_H
#define OUTSTRMFMT_H

#include <inttypes.h>
#include <stdio.h>

class outstrmfmt
{
public:
    outstrmfmt(){};
    virtual ~outstrmfmt(){};
    virtual bool init(int,int)=0;
    virtual uint32_t convert420(const uint8_t* fmt420, int insz, int w,int h, int jpg_quality, uint8_t** ppng)=0;
    virtual uint32_t convertBW(const uint8_t* uint8buf, int insz, int w, int h, int jpg_quality, uint8_t** pjpeg)=0;
protected:

private:
};


#define DERR() "Error: " << errno << "\n";

#endif // OUTSTRMFMT_H
