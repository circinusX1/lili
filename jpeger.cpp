/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/

    Author:  Marius O. Chincisan
    First Release: September 16 - 29 2016
*/
#include <iostream>
#include <assert.h>
#include "jpeger.h"
#include "cbconf.h"


#define BLOCK_SZ	16384
extern bool __alive;

typedef struct
{
    struct jpeg_destination_mgr pub;
    JOCTET *buf=nullptr;
    size_t bufsize=0;
    size_t jpegsize=0;
} mem_destination_mgr;

typedef mem_destination_mgr *mem_dest_ptr;

static void     _init_destination(j_compress_ptr cinfo);
static boolean  _empty_output_buffer(j_compress_ptr cinfo);
static void     _term_destination(j_compress_ptr cinfo);
static int      _jpeg_mem_size(j_compress_ptr cinfo);
static jpeger*  _pthis;

jpeger::jpeger(int q, bool bw):_image(0),_jpgq(q),_imgsize(0),_memsz(0)
{
    _pthis=this;
    ::memset(&_cinfo, 0, sizeof(_cinfo));
    _cinfo.err = ::jpeg_std_error(&_jerr);
    jpeg_create_compress(&_cinfo);
    _jpeg_mem_dest(&_cinfo);
    _bandw = bw;
}

jpeger::~jpeger()
{
    jpeg_destroy_compress(&_cinfo);
    free (_image);  //dtor
}

bool jpeger::init(const dims_t&)
{
    return true;
}

uint32_t jpeger::convert420(const uint8_t* fmt420, int insz,
                            int w, int h,
                             const uint8_t** pjpeg)
{
    const uint8_t* pstart = fmt420;
    (void)insz;
    _imgsize =  _put_jpeg_yuv420p_memory(pstart, w, h, _jpgq, 0);
    *pjpeg = _image;
    return  _imgsize;
}

uint32_t jpeger::convertBW(const uint8_t* uint8buf, int insz, int w, int h, const uint8_t** pjpeg)
{
    (void)insz;
    if(_memsz)
    {
        JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
        int row_stride;

        memset(_image,0xFF,_memsz);

        _cinfo.image_width = w;
        _cinfo.image_height = h;
        _cinfo.input_components = 1;
        _cinfo.in_color_space = JCS_GRAYSCALE;
        jpeg_set_defaults(&_cinfo);
        jpeg_set_colorspace(&_cinfo, JCS_GRAYSCALE);
        _cinfo.dct_method = JDCT_FASTEST;
        jpeg_set_quality(&_cinfo, 80, TRUE);
        jpeg_start_compress(&_cinfo, TRUE);
        row_stride = w ;

        while (_cinfo.next_scanline < _cinfo.image_height)
        {
            row_pointer[0] = (unsigned char*)&uint8buf[_cinfo.next_scanline * row_stride];
            (void) jpeg_write_scanlines(&_cinfo, row_pointer, 1);
        }

        jpeg_finish_compress(&_cinfo);
        int jpeg_imgsz = _jpeg_mem_size(&_cinfo);
        *pjpeg = _image;
        return jpeg_imgsz;
    }
    return 0;
}


int jpeger::_put_jpeg_yuv420p_memory(const uint8_t *pyuv420,
                                     int width,
                                     int height,
                                     int jpg_quality,
                                     struct tm *tm)
{
    int i, j, jpeg_imgsz;
    (void)tm;

    _cinfo.image_width = width;
    _cinfo.image_height = height;
    _cinfo.input_components = 3;
    jpeg_set_defaults(&_cinfo);
    jpeg_set_colorspace(&_cinfo, JCS_YCbCr);
    _cinfo.raw_data_in = TRUE;
#if JPEG_LIB_VERSION >= 70
    _cinfo.do_fancy_downsampling = FALSE;
#endif
    _cinfo.comp_info[0].h_samp_factor = 2;
    _cinfo.comp_info[0].v_samp_factor = 2;
    _cinfo.comp_info[1].h_samp_factor = 1;
    _cinfo.comp_info[1].v_samp_factor = 1;
    _cinfo.comp_info[2].h_samp_factor = 1;
    _cinfo.comp_info[2].v_samp_factor = 1;
    jpeg_set_quality(&_cinfo, jpg_quality, 0);
    _cinfo.dct_method = JDCT_FASTEST;

    if(_bandw)
    {
        _cinfo.input_components = 1;
        _cinfo.in_color_space = JCS_GRAYSCALE;
        jpeg_set_defaults(&_cinfo);
        jpeg_set_colorspace(&_cinfo, JCS_GRAYSCALE);
        _cinfo.dct_method = JDCT_FASTEST;
        jpeg_set_quality(&_cinfo, 80, TRUE);
    }

    jpeg_start_compress(&_cinfo, TRUE);
    do{
        if(_bandw)
        {
            JSAMPROW row_pointer[1];
            int row_stride = width ;
            while (_cinfo.next_scanline < _cinfo.image_height)
            {
                row_pointer[0] = (unsigned char*)&pyuv420[_cinfo.next_scanline * row_stride];
                (void) jpeg_write_scanlines(&_cinfo, row_pointer, 1);
            }
        }
        else{

            JSAMPROW        y[16],cb[8],cr[8];
            JSAMPARRAY      data[3];
            data[0] = y;
            data[1] = cb;
            data[2] = cr;

            for (j = 0; j < height; j += 16)
            {
                for (i = 0; i < 16; i++)
                {
                    y[i] = ( unsigned char*)pyuv420 + width * (i + j);

                    if (i % 2 == 0)
                    {

                        cb[i / 2] = ( unsigned char*)pyuv420 + width * height + width / 2 * ((i + j) /2);
                        cr[i / 2] = ( unsigned char*)pyuv420 + width * height + (width * height / 4)
                                +  ((width / 2) * ((i + j) / 2) );

                    }
                }
                jpeg_write_raw_data(&_cinfo, data, 16);
            }
        }
    }while(0);


    jpeg_finish_compress(&_cinfo);
    jpeg_imgsz = _jpeg_mem_size(&_cinfo);

    return jpeg_imgsz;
}

void jpeger:: _jpeg_mem_dest(j_compress_ptr cinfo)
{
    mem_dest_ptr dest;

    if (cinfo->dest == NULL)
    {
        cinfo->dest = (struct jpeg_destination_mgr *)
                (*cinfo->mem->alloc_small)((j_common_ptr)cinfo,
                                           JPOOL_PERMANENT,
                                           sizeof(mem_destination_mgr));
    }
    dest = (mem_dest_ptr) cinfo->dest;
    dest->pub.init_destination    = _init_destination;
    dest->pub.empty_output_buffer = _empty_output_buffer;
    dest->pub.term_destination    = _term_destination;

    dest->buf      = _image = (uint8_t*)malloc(BLOCK_SZ);
    assert(dest->buf);
    dest->bufsize  = _memsz = BLOCK_SZ;
    dest->jpegsize = 0;

    _init_destination(&_cinfo);
    TRACE() << "      JPEGER MEM DEST \r\n";

}

static void  _init_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    dest->pub.next_output_byte  = dest->buf;
    dest->pub.free_in_buffer    = dest->bufsize;
    dest->jpegsize = 0;

    // TRACE() << "      JPEGER INIT TO "<<dest->buf <<","<<dest->bufsize << "bytes \r\n";
}

static boolean  _empty_output_buffer(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;

    size_t oldsize = _pthis->_memsz;
    uint8_t* nb = (uint8_t*)::malloc(oldsize + BLOCK_SZ);
    if(nb==nullptr){
        TRACE()<<__FUNCTION__<<" out of memory \r\n";
        return FALSE;
    }
    ::memcpy(nb, _pthis->_image, oldsize);
    ::free(_pthis->_image);
    _pthis->_image = nb;
    _pthis->_memsz = oldsize+BLOCK_SZ;

    dest->pub.next_output_byte = _pthis->_image  + oldsize;
    dest->pub.free_in_buffer = BLOCK_SZ;
    dest->buf = _pthis->_image;
    dest->bufsize = _pthis->_memsz;

    TRACE() << "      JPEGER REALLOC TO "<<dest->buf <<","<<dest->bufsize << "bytes \r\n";
    return TRUE;
}

static void _term_destination(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;
    size_t jpgsize = dest->bufsize -cinfo->dest->free_in_buffer;
    dest->jpegsize = jpgsize;

    // TRACE() << "      JPEGER TERM \r\n";
}

static int _jpeg_mem_size(j_compress_ptr cinfo)
{
    mem_dest_ptr dest = (mem_dest_ptr) cinfo->dest;

    // TRACE() << "      JPEG SZ "<<dest->jpegsize<<"\r\n" ;

    return dest->jpegsize;
}
