#ifndef ENCRYPTER_H
#define ENCRYPTER_H


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "cbconf.h"

using namespace std;

class Encryptor
{
private:
    std::string   _key;

public:
    Encryptor()
    {
        _key = CFG["webcast"]["security"].value();
    }
    void encrypt(unsigned int val, uint8_t* out)
    {
        char text[32];
        sprintf(text,"%u",val);
        out[0]=0;
        for(int i=0; text[i]; i++)
        {
            out[i+1] += text[i]+_key[i%_key.length()];
            out[0]++;
        }
        TRACE() << "key:" << _key << " enc:" << out+1 << "\r\n";
    }

    unsigned int  decrypt(const uint8_t* text)
    {
        if(_key.empty())
            return 0;
        char    loco[16] = {0};
        uint8_t len = text[0];
        const uint8_t* pc = text+1;
        for(int i=0; i<len; i++)
        {
            loco[i] = pc[i] - _key[i%_key.length()];
        }
        return ::atoi((const char*)loco);
    }

};

#endif // ENCRYPTER_H
