#ifndef CAMEVENTS_H
#define CAMEVENTS_H

#include "motion.h"
#include "cbconf.h"
#include "lilitypes.h"

#define FLAG_RECORD     0x1
#define FLAG_WEBCAST    0x2
#define FLAG_TIMELAPSE  0x4
#define FLAG_SAVE       0x8
#define FLAG_FORCE_SAVE  0x10

struct imglayout_t{
    const uint8_t *_camp = nullptr;
    size_t         _caml = 0;
    EIMG_FMT       _camf = e422;
    const uint8_t *_jpgp = nullptr;
    size_t         _jpgl = 0;
    EIMG_FMT       _jpgf = eFJPG;
};

class camevents
{
public:
    camevents(const dims_t& wh, const Cbdler::Node& n);

    void proc_events(const imglayout_t& cam);
    const uint8_t* getm(int& w, int& h, int& sz);
    void clean_events();

private:
    int _darkaverage()const{return _mt ?  _mt->darkav() : 0;}
    int _proc_events(const uint8_t* buff, size_t len, EIMG_FMT fmt);

private:
    mmotion*    _mt             = nullptr;
    time_t      _lasttime       = 0;
    int         _robinserve     ;
    time_t      _mpgnewfile     ;
    time_t      _lapsetick      ;
    time_t      _tickmove       ;
    event_t     _event          ;
    dims_t      _mohilo         ;
    int         _innertia       ;
    int         _inertiiaitl    ;
    int         _time_lapse     ;
    int         _dark_lapse     ;
    int         _dark_motion    ;
    int         _one_shot       ;
    std::string _save_loc       ;
    int         _max_size       ;
    int         _on_max_files   ;
    std::string _on_max_comand  ;
    std::string _run_app          ;
    int         _movementintertia ;
    int         _firstimage       ;
};

#endif // CAMEVENTS_H
