#ifndef ENCRYPTER_H
#define ENCRYPTER_H


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "config.h"
#include "main.h"

using namespace std;

class Encryptor
{
private:
    std::string   _key;
    char          _text[24];
public:
    Encryptor()
    {
        _key = CFG["security"].value();
    }

    void encrypt(unsigned int val,uint8_t* out)
    {
        if(_key=="XXXX"){
            return ;
        }
        char text[32];
        sprintf(text,"%u",val);
        out[0]=0;
        for(int i=0; text[i]; i++)
        {
            out[i+1] += text[i]+_key[i%_key.length()];
            out[0]++;
        }
    }

    unsigned int  decrypt(const uint8_t* text)
    {
        if(_key=="XXXX"){
            return 0;
        }

        char    loco[32] = {0};
        uint8_t len = text[0];
        const uint8_t* pc = text+1;
        if(len>16){
            return 0;
        }

        for(int i=0; i<len; i++)
        {
            loco[i] = pc[i] - _key[i%_key.length()];
        }
        return ::atoi((const char*)loco);
    }

};

#endif // ENCRYPTER_H
