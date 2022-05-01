///   EXPERIMENTAL

#include "anytojpg.h"
#include "cbconf.h"


mpeger::mpeger(int q, bool)
{
    (void)q;
}

mpeger::~mpeger()
{

}

bool mpeger::init(const dims_t& imgsz)
{

    return true;
}

int mpeger::cam_to_jpg(imglayout_t& img)
{
    if(!_oneinit)
    {
        FILE* pf = fopen("tmp/img.mpg","ab");
        if(pf){
            ::fwrite(img._camp,1, img._caml, pf);
            ::fclose(pf);
        }
        //_oneinit=true;
    }
    return 0;
}

int mpeger::cam_to_bw(imglayout_t&)
{
    return 0;
}
