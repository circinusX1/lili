#ifndef CONFIG_H
#define CONFIG_H

#include <dlfcn.h>

/**
  Curly Bracket Document Layout  CBDL

Copyright (C) [2020] by marius chincisan marrius9876@gmail.com

Permission to use, copy, modify, and/or distribute this software for any purpose
with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS
OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
THIS SOFTWARE.

*/
#include <assert.h>
#include <iostream>
#include <assert.h>
#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include <utility>
#include <sstream>
#include <fstream>
#include "lilitypes.h"

struct Nullout
{
    Nullout& operator <<(uint8_t*){return *this;}
    Nullout& operator <<(const char*){return *this;}
    Nullout& operator <<(size_t){return *this;}
    Nullout& operator <<(const std::string&){return *this;}
};

std::string base64_encode(unsigned char const* bytes_to_encode, unsigned int in_len);


extern Nullout     nullout;


#define TRACE()   std::cout




class Cbdler
{
public:
    static std::string _last_line;
    static int         _ilast_line;

    class Node
    {
    public:
        friend class Cbdler;

    protected:
        enum E_TYPE{eNULL,eNODE,eLEAF};
        Node(Node::E_TYPE t, const std::string& name):_name(name),_type(t){
            _parent=nullptr;
        }
        Node(Node::E_TYPE t=eNULL):_type(t){
            _parent=nullptr;
        }
        ~Node(){
            _name.clear();
            for(const auto& a : _values)
                delete a;
            _values.clear();
        }
        const std::string& nv()const{return _name;};
        bool ok()const{return _type!=eNULL;}
        void name_it(std::string& name){
            _name = name;
            name.clear();
        }

        Node* store_it(std::string& name){
            Node* paka = new Node(Node::eLEAF, name);
            paka->_parent = this;
            _type = eNODE;
            this->_values.push_back(paka);
            name.clear();
            return paka;
        }
public:
        const Node* top()const{
            return _root();
        }

        void  add(Node* pn){
            this->_type = Node::eNODE;
            this->_values.push_back(pn);
        }

        const Node* add(const char* value){
            std::string sv(value);
            store_it(sv);
            return this;
        }

        const Node& operator[](const char* key)const
        {
            if(key==_name)
                return *this;
            for(const auto& a : _values)
            {
                if(a->_name==key){
                    if(a->_values.size()==1 && a->_values[0]->_name[0]=='@')
                    {
                        std::string idx = "0";
                        const Node* pn = _get_ref(this, a->_values[0]->_name,  idx);
                        return *pn;
                    }
                    // cannot have parent and first child
                    assert(this->_name!=key);
                    return *a;
                }
            }

            static Node Dummy;
            return Dummy;
        }

        bool is_node()const{return _type==Node::eNODE;}

        const Node& scope(size_t index=0)const{
            static Node empty(Node::eNULL,"");
            if(_values.size() && _values.size()>=index)
                return *_values[index];
            return empty;
        }

protected:
        const Node* _get_ref(const Node* pn, const std::string& v, std::string& idx)const
        {
            bool indexing=false;
            std::string ev;
            for(const auto& a : v)
            {
                if(a=='@')
                    continue;
                if(a=='&')
                    continue;
                if(a=='/'){
                    if(ev==".."){
                        pn=pn->_parent;
                    }else{
                        if(ev.empty()){
                            pn=pn->_root();
                        }
                        else{
                            pn=pn->_at(ev);
                        }
                    }
                    ev.clear();
                    continue;
                }else{
                    if(a=='['){
                        indexing=true;
                        continue;
                    }
                    if(a==']'){
                        indexing=false;
                        continue;
                    }
                }
                if(indexing)
                    idx+=a;
                else
                    ev += a;
            }//for
            pn = pn->_at(ev);
            return pn;
        }

        const Node* parent(){
            return _parent;
        }

        const std::vector<Node*>& values(){
            return _values;
        }

        const Node& node(size_t index=0)const{
            static Node empty(Node::eNULL,"");
            if(_values.size() && _values.size()>=index)
                return *_values[index];
            return empty;
        }

public:
        const Node* pnode(const char* s)const {
            std::string ss = s;
            return _at(ss);
        }

        const std::string value(size_t index=0)const{
            static std::string empty="";
            if(_values.size() )
            {
                std::string v;
                std::string idx = "0";
                if(_values.size()>index){
                    v = _values[index]->_name;
                }
                else{
                    v = _values[0]->_name;
                    idx = std::to_string(index);
                }

                if(v[0]!='@') // value reference
                    return v;

                const Node* pn = this;

                // @../../rect/sss/sss[2],@/x/sdf/size
                pn = _get_ref(pn, v, idx);
                if(pn!=nullptr)
                {
                    size_t dx = !idx.empty() ? std::stod(idx) : 0;
                    if(pn->_values.size()>dx)
                        return pn->_values[dx]->_name;
                }
            }
            return empty;
        }
        bool exist()const {return this->_values.size()>0;}
        const std::string to_string(size_t index=0)const{return value(index);}
        int to_int(size_t index=0)const{return ::atoi(to_string(index).c_str());}
        rect_t to_rect(int index=0)const{
            rect_t rt;
            rt.x = to_int(index);
            rt.y = to_int(1);
            rt.X = to_int(2);
            rt.Y = to_int(3);

            return rt;
        }
        dims_t to_dims(int index=0)const{
            dims_t rt;
            rt.x = to_int(index);
            rt.y = to_int(1);
            return rt;
        }

        size_t count()const{
            return _values.size();
        }

private:
        const Node* _root()const {
            const Node* pn = this;
            while(pn->_parent)
                pn=pn->_parent;
            return pn;
        }

        const Node* _at(const std::string& s)const {
            for(const auto& a : _values){
                if(a->_name==s){
                    return a;
                }
            }
            static Node empty;
            return &empty;
        }

        void _del_node(Node* pn)
        {
            std::vector<Node*>::iterator i = _values.begin();
            for(;i!=_values.end();++i){
                if((*i) == pn)
                {
                    delete pn;
                    _values.erase(i);
                    break;
                }
            }
        }

    private:
        std::string                  _name;
        std::vector<Node*>           _values;
        Node*                        _parent;
        Node::E_TYPE                 _type;
    };

public:
    Cbdler():_pnode(nullptr)
    {

    }
    // looup todo
    const Cbdler::Node& operator[](const char* key)const
    {
        return _pnode->operator[](key);
    }
    Node* parse(const char* fname)
    {
        if(_pnode)
            delete _pnode;
        _pnode = nullptr;
        return _parse(fname);
    }

    // construct  a tree
    Node* begin(const char* n)
    {
        assert(_pnode == nullptr);
        _pnode = new Node(Node::eNODE, n);
        return _pnode;
    }

    Node* make(const char* n){return new Node(Node::eNODE, n);}
    Node* root()const {return _pnode;}

private:
    Node* _parse(const char* fname)
    {
        int   line = 1;
        int   oc   = 0;
        int   eq   = 0;
        int   els  = 0;
        bool  longstr = false;
        bool  escaping = false;
        Node* paka = nullptr;
        Node* parent = nullptr;
        std::ifstream fi(fname);
        if(fi.is_open())
        {
            while (std::getline(fi, _line))
            {
                ++line;
                Cbdler::_last_line = _line;
                Cbdler::_ilast_line = line-1;

                std::istringstream iss(_line);
                if(!escaping && !longstr){
                    if(_line.empty() || _line[0]=='#')
                        continue;
                    if(eq){
                        TRACE()<<" line:" << line << " missing ; at the end when using = or : \n";                    throw 2;
                    }
                }
                if(longstr){
                    _line+="\n";
                }

                for(const auto f : _line)
                {
                    if(f=='#'){ _line.empty(); break; }
                    switch(f)
                    {
                    case '\t':
                    case ' ':
                        if(longstr){
                            _string+=f;
                        }
                        break;
                    case '\\':
                        if(escaping)
                        {
                            _string+=f;
                            escaping=false;
                        }
                        else{
                            escaping=true;
                        }
                        break;
                    case '\"':
                        if(escaping){
                            _string+=f;
                            escaping=false;
                        }
                        else
                            longstr = !longstr;
                        break;
                    case ':':
                    case '=':
                        eq=true;
                        [[fallthrough]];
                    case '{':
                        if(escaping){
                             _string+=f;
                            escaping=false;
                            break;
                        }
                        // doc has no root, make one and link it to curent first one
                        if(els==1 && paka==nullptr)
                        {
                            Node* root   = new Node(Node::eNODE,"");
                            Node* curent = _pnode;
                            root->add(curent);
                            curent->_parent = root;
                            _pnode = root;
                            paka = root;
                        }
                        parent = paka;
                        paka = _new();
                        paka->_name=_string;
                        paka->_parent = parent;
                        _string.clear();
                        if(paka!=parent && parent)
                            parent->_values.push_back(paka);
                        ++oc;
                        break;
                    case '}':
                    case ';':
                        eq=false;
                        if(escaping){
                             _string+=f;
                            escaping=false;
                            break;
                        }
                        if(paka==nullptr){
                            throw line;
                        }
                        if(!_string.empty())
                        {
                            if(!paka->_name.empty())
                            {
                                if(paka->_name=="%include")
                                {
                                    Cbdler sub;
                                    Node* pn = sub.parse(_string.c_str());
                                    if(pn)
                                    {
                                        if(_pnode==nullptr)
                                        {
                                            _pnode = pn;
                                        }
                                        else {
                                            Node* prent = paka->_parent ? paka->_parent : _pnode;
                                            prent->_del_node(paka);
                                            pn->_parent = prent;
                                            prent->add(pn);
                                        }
                                    }
                                    else {
                                        std::string subfn = _string;
                                        _string="~failed to load ";
                                        _string+=subfn;
                                        paka->store_it(_string);
                                    }
                                    _wipe();
                                }
                                else
                                {
                                    paka->store_it(_string);
                                }
                            }
                        }
                        paka = paka->_parent;
                        ++els;
                        --oc;
                        break;
                    case ',':
                        if(escaping){
                             _string+=f;
                            escaping=false;
                            break;
                        }
                        paka->store_it(_string);
                        break;
                    default:
                        _string+=f;
                        break;
                    }
                }
            }
        }else {
            TRACE()<<"error open " << fname << "\n";
        }
        if(oc || _pnode==nullptr){
            TRACE()<<"\r\n document malformated \r\n";
            throw(Cbdler::_ilast_line);
        }
        return _pnode;
    }

    Node* _new()
    {
        if(_pnode==nullptr)
        {
            _pnode=new Node(Node::eNODE);
            _pnode->_parent = _pnode;
            return _pnode;
        }
        return new Node(Node::eLEAF);
    }


    void _wipe()
    {
        TRACE() << _string.c_str() << "\n";
        _string.clear();
    }

public:
    void print(const Node* p, int depth)
    {
        ++depth;
        TRACE() << "\n";
        for(int i=0;i<depth;i++) TRACE()<<"    ";
        TRACE()  << p->_name.c_str();
        if(p->_values.size())
        {
            TRACE() << "\n";
            for(int i=0;i<depth;i++) TRACE()<<"    ";
            TRACE() << "{\n";

            for(const auto& a : p->_values)
            {
                print(a,depth);
            }
            TRACE() << "\n";
            for(int i=0;i<depth;i++) TRACE()<<"    ";
            TRACE() << "}\n";
        }
        --depth;
    }

private:

    std::string             _string;
    std::string             _line;
    Node*                   _pnode = nullptr;
    bool                    _escap=false;
    bool                    _toking=false;
    bool                    _refon=false;
};

extern Cbdler*  pCFG;
#define CFG (*pCFG)


inline void* dls(void* v, const char* fn)
{
    TRACE() << fn << "\n";
    void* pr =  ::dlsym(v,fn);
    if(pr==nullptr)
    {
        char* error;
        if ((error = dlerror()) != NULL)  {
            TRACE() << error << "\n";
            return nullptr;
        }

    }
    return pr;
}


#define SO_SYM(lib_,foo) _p##foo = (p_##foo)::dls(lib_,static_cast<const char*>(#foo)); \
                         assert(_p##foo);




#endif // CONFIG_H
