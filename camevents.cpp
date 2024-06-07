
#include "lilitypes.h"
#include "camevents.h"

/*
;int motionlow,
                           int motionhi, const dims_t& wh,
                           const rect_t& inr,
                           const rect_t& outr):_mt(wh.x, wh.y, inr, outr)
                           */

camevents::camevents(const dims_t& wh, const Cbdler::Node& np)
{

    const Cbdler::Node& n = np["move"];

    //int motionlow, int motionhi, const dims_t& wh
    _robinserve     = 0x1;
    _mpgnewfile     = 0;
    _lapsetick      =  gtc();
    _tickmove       =  gtc();
    _event          = {0,0};
    _mohilo         = n["motion"].to_dims();
    _innertia       = n["innertia"].to_int(0);
    _inertiiaitl    = n["innertia"].to_int(1);
    _time_lapse     = n["time_lapse"].to_int();
    _dark_motion    = n["dark_limit"].to_int(0);
    _save_loc       = n["save_loc"].value();
    _max_size       = n["max_movie"].to_int();
    _on_max_files   = n["on_max_files"].to_int(0);
    _on_max_comand  = n["on_max_files"].value(1);
    _run_app        = n["run_app"].value();
    _movementintertia = 0;
    _firstimage = 0;
    if(_mohilo.x && _mohilo.y)
    {
        _mt = new mmotion(wh, n);
    }

}

int camevents::_proc_events(const imglayout_t& imgl)
{
    if(_mt)
    {
        if(imgl._camf==e422){
            int movedpix =  _mt->det_mov(imgl, _mohilo);
            if(movedpix > _mohilo.x && movedpix < _mohilo.y)
            {
                TRACE() << "moved: " << movedpix << "\n";
                return uint8_t(movedpix);
            }
        }
    }
    return 0;
}


const uint8_t* camevents::getm(int& w, int& h, int& sz)
{
    if(_mt){
        _lasttime = time(0);
        w  = _mt->getw();
        h  = _mt->geth();
        sz = w * h;
        return _mt->motionbuf();
    }
    return nullptr;
}

const event_t&  camevents::proc_events(const imglayout_t& imgl,
                                       const std::string&,
                                       uint8_t prep_pred)
{
    uint32_t now =  gtc();

    _event.predicate |= prep_pred;
    if(now - _tickmove > _inertiiaitl)
    {
        if(_mt){
            _event.movepix = _mt->det_mov(imgl,_mohilo);
        }

        if( _event.movepix )
        {
            _movementintertia = _innertia;
        }
        if(_movementintertia > 0)
        {
            --_movementintertia;
            if(_event.movepix==0){
                _event.movepix = _movementintertia;
            }
        }
        if(_event.movepix){
            TRACE() << "MOTIONEV = movpix = " << _event.movepix << "\n";
            _event.predicate |= EVT_MOTION;

        }else{
            _event.predicate &= ~EVT_MOTION;
        }
        _tickmove = now;
    }

    if(_time_lapse > 0)
    {
        if((now - _lapsetick) > (uint32_t)_time_lapse)
        {
            _event.predicate |= EVT_TLAPSE;
            _lapsetick = now;
        }else{
            _event.predicate &= ~EVT_TLAPSE;
        }
    }

    if(_mt && _mt->darkav() < _dark_motion)
    {
        _event.predicate &= ~EVT_TLAPSE;
        _event.predicate &= ~EVT_MOTION;
    }

    if((_event.predicate & EVT_JUST_MOTION) != 0 &&
        !_run_app.empty())
    {
        ::system(_run_app.c_str());
    }

    return _event;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void camevents::clean_events()
{
    _event.movepix = 0;
    _event.predicate = 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void camevents::save_local(const imglayout_t& imgl,
                           const std::string&,
                           uint8_t ,
                           const std::string& folder)
{
    if((_event.predicate & FLAG_SAVE && _event.movepix) ||
            (_event.predicate & FLAG_FORCE_SAVE) || !folder.empty())
    {
        char fname[256];
        std::string savloc = _save_loc;

        if(!folder.empty()){
            savloc = folder;
        }

        if(imgl._camf == eNOTJPG)
        {
            if(time(0) - _mpgnewfile > 3600*24)  /*every day*/
            {
                _mpgnewfile = time(0);
                ::sprintf(fname, "%smov_%d.mpeg", savloc.c_str(), (int)_mpgnewfile);
                FILE* 		pff = ::fopen(fname,"wb");
                if(pff){
                    ::fclose(pff);
                }
            }
            else
            {
                ::sprintf(fname, "%smov_%d.mpeg", savloc.c_str(), int(_mpgnewfile));
                FILE* 		pff = ::fopen(fname,"ab");
                long        flen = 0;
                if(pff)
                {
                    ::fseek(pff,0,SEEK_END);
                    flen = ::ftell(pff);
                    ::fwrite(imgl._camp,1,imgl._caml,pff);
                    ::fclose(pff);
                }
                if(flen > (long) (1000000 * _max_size)){
                    _mpgnewfile = 0;          /*every exceed*/
                }
            }
        }
        else if(imgl._jpgf == eFJPG && _on_max_files>0)
        {
            ::sprintf(fname, "%si%04d-%06d.jpg", savloc.c_str(),_event.movepix, _firstimage);
            ++_firstimage;
            if(_firstimage > _on_max_files)  _firstimage = 0;
            FILE* 		pff = ::fopen(fname,"wb");
            if(pff)
            {
                ::fwrite(imgl._jpgp,1,imgl._jpgl,pff);
                ::fclose(pff);
                TRACE() << "saving: " << fname << "\n";
                ::symlink(fname,"tmp/lastimage.jpg");
            }
        }
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void camevents::set(const dims_t& mohilo, int pixnoise, int pixdiv, int imgscale)
{
    if(_mt)
    {
        if(mohilo.x > 0){
            _mohilo = mohilo;
        }
        _mt->set(pixnoise, pixdiv, imgscale);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////
void camevents::get(dims_t& mohilo, int& pixnoise, int& pixdiv, int& imgscale)
{
    if(_mt)
    {
        mohilo = _mohilo;
        _mt->get(pixnoise, pixdiv, imgscale);
    }

}
