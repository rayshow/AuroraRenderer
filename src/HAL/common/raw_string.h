#pragma once

#include<string>
#include"type.h"

PROJECT_NAMESPACE_BEGIN

template<typename Char>
struct RawString;


template<>
struct RawString<char>
{
    static uint64 length(char const* rawString){
        if(rawString == nullptr) return 0;
        return strlen(rawString);
    }

    static char* copy(char const* rawString){
        if(rawString == nullptr) return nullptr;
        return strdup(rawString);
    }

    static char* copy(char*& dest, char const* src){
        if(src == nullptr) return nullptr;
        return strcpy(dest, src);
    }
};

using RawCharString = RawString<char>;

PROJECT_NAMESPACE_END