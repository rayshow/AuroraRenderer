#pragma once

#include<string>
#include<vector>
#include<algorithm>
#include<variant>
#include<type_traits>
#include<array>
#include<unordered_map>
#include<unordered_set>
#include"core/type.h"

PROJECT_NAMESPACE_BEGIN

#define AR_APP_CONFIG_TYPE i64, f64, i32, f32, bool, void*, AString, String, Path
    using AppConfigsVeriant = std::variant<AR_APP_CONFIG_TYPE>;
template<typename T>
concept AppConfigsType = is_one_of_v<T, AR_APP_CONFIG_TYPE>;
#undef AR_ALLOW_TYPE

class AppConfigs
{
public:
    enum
    {
        Commandline,
        BinDir,
        AppHandle,
        AppName,
        WinHandle,
        WinWidth,
        WinHeight,
        WinResizable,
        VSync,
        InnerDataDir,
        ExternalDataDir,
        TempDir,
        Max = 32,
    };


    template<typename T>
    T const& widthDefault(T* pt, T const& inDefault)
    {
        return pt ? *pt : inDefault;
    }

    template<AppConfigsType T>
    T const& get(i32 index, T const& inDefault = {}) {
        return widthDefault<T>( std::get_if<T>(&predefinedConfigs[index]), inDefault);
    }

    template<AppConfigsType T>
    T const& get(String const& name, T const& inDefault) {
        auto found = configs.find(name);
        if (found == configs.end()) {
            return nullptr;
        }
        return widthDefault<T>(std::get_if<T>(*found), inDefault) ;
    }
    
    template<AppConfigsType T >
    void set(i32 index, T const& t) {
        ARAssert(index >= 0 && index < Max);
        predefinedConfigs[index] = t;
    }

    template<AppConfigsType T>
    void set(String const& name, T const& t) {
        configs.insert_or_assign(name, t);
    }

    template<AppConfigsType T>
    void set(String && name, T&& t) {
        configs.insert_or_assign(std::move(name), std::move(t));
    }

    void addSwitch(String const& name) {
        switches.emplace(name);
    }

    bool hasSwitch(String const& name) {
        return switches.contains(name);
    }

private:
    TSet<String>                                      switches{};
    TStaticArray< AppConfigsVeriant, AppConfigs::Max> predefinedConfigs{};
    TMap<String, AppConfigsVeriant>                   configs{};
};
inline AppConfigs GAppConfigs;

PROJECT_NAMESPACE_END