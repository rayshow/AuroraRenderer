#pragma once

template<typename T>
struct Singleton
{
    using Type = T;
private:
    Singleton() = default;
    Singleton(const Singleton&) = default;
public:
    
    static T& getInstance(){
        static T t{};
        return t;
    }
};