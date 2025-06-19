#pragma once

template<typename T>
struct Singleton
{
protected:
    Singleton() = default;
public:
    using Type = T;
    static T& getInstance(){
        static T t{};
        return t;
    }
    
    Singleton(Singleton&&) = delete;
    Singleton(Singleton const&) = delete;
    void operator=(Singleton const&) = delete;
    void operator=(Singleton &&) = delete;
};