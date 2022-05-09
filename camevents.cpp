
#include "lilitypes.h"
#include "camevents.h"

/*
;int motionlow,
                           int motionhi, const dims_t& wh,
                           const rect_t& inr,
                           const rect_t& outr):_mt(wh.x, wh.y, inr, outr)
                           */
///////////////////////////////////////////////////////////////////////////////////////////////////
camevents::camevents(const dims_t& wh, const Cbdler::Node& n)
{
    //int motionlow, int motionhi, const dims_t& wh
    _robinserve     = 0x1;
    _mpgnewfile     = 0;
    _event          = {0,0};
    _mohilo         = n["motion"].to_dims();
    _innertia       = n["innertia"].to_int(0);
    _inertiiaitl    = n["innertia"].to_int(1);
    _lapsetime      = n["time_lapse"].to_int();
    _dark_lapse     = n["dark_comp"].to_int(0);
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

///////////////////////////////////////////////////////////////////////////////////////////////////
const uint8_t* camevents::getm(int& w, int& h, int& sz)
{
    if(_mt){
        w  = _mt->getw();
        h  = _mt->geth();
        sz = w * h;
        return _mt->motionbuf();
    }
    return nullptr;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
const event_t&  camevents::proc_events(const imglayout_t& imgl,
                                        std::string& name, uint8_t prep_pred)
{
    time_t diffc  =  (imgl._now - _movetime) + 1;
    time_t diffl  =  imgl._now - _lapsetime;

    _event.predicate |= prep_pred;

    if(diffc > _inertiiaitl)
    {
        _movetime = imgl._now;

        if(_mt){
            _event.movepix = _mt->det_mov(imgl);
            if(_event.movepix  < _mohilo.x || _event.movepix  > _mohilo.y)
            {
                _event.movepix = 0;
            }
        }

        if( _event.movepix )
        {
            _movementintertia = _innertia;
        }
        if(_movementintertia > 0)
        {
            _movementintertia--;
            if(_event.movepix==0){
                _event.movepix = (_mohilo.x+1);
            }
        }
        if(_event.movepix){
            _event.predicate |= EVT_MOTION;
        }else{
            _event.predicate &= ~EVT_MOTION;
        }
    }

    if(_lapsetime)
    {
        if(diffl > (time_t)_lapsetime)
        {
            _event.predicate |= EVT_TLAPSE;
        }
        else
        {
            _event.predicate &= ~EVT_TLAPSE;
        }
        _lapsetime = imgl._now;
        TRACE()<< name << " timelaspe\n";
    }

    if(_mt && _mt->darkav() < _dark_lapse)
    {
        _event.predicate &= ~EVT_TLAPSE;
        _event.predicate &= ~EVT_MOTION;
    }

    if(_event.predicate & EVT_MOTION)
    {
        TRACE()<< name << " move:" << _event.movepix << "\n";
    }

    if((_event.predicate & EVT_JUST_MOTION) != 0){
        ::system(_run_app.c_str());
    }

    if((_event.predicate & FLAG_SAVE && _event.movepix) ||
            _event.predicate & FLAG_FORCE_SAVE)
    {
        char fname[256];

        if(imgl._camf == eNOTJPG)
        {
            if(time(0) - _mpgnewfile > 3600*24)  /*every day*/
            {
                _mpgnewfile = time(0);
                ::sprintf(fname, "%smov_%d.mpeg", _save_loc.c_str(), (int)_mpgnewfile);
                FILE* 		pff = ::fopen(fname,"wb");
                if(pff){
                    ::fclose(pff);
                }
            }
            else
            {
                ::sprintf(fname, "%smov_%d.mpeg", _save_loc.c_str(), int(_mpgnewfile));
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
        else if(imgl._jpgf == eFJPG)
        {
            ::sprintf(fname, "%si%04d-%06d.jpg", _save_loc.c_str(),_event.movepix, _firstimage);
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
    return _event;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
void camevents::clean_events()
{
    _event.movepix = 0;
    _event.predicate = 0;
}
