#pragma once

template<typename T>
struct Singleton
{
public:
    using Type = T;
    static T& getInstance(){
        static T t{};
        return t;
    }

};