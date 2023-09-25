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

template<typename T, typename... Args>
struct is_one_of : std::disjunction< std::is_same<T, Args>...> {};

class AppConfigs
{
public:
    enum
    {
        Commandline,
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
    using AppConfigVarient = std::variant<i64, f64, i32, f32, bool, void*, String>;
    template<typename T>
    using right_type = is_one_of<T, i64, f64, i32, f32, bool, void*, String>;
    template<typename T>
    static constexpr bool right_type_v = right_type<T>::value;
    template<typename T>
    using check_t = std::enable_if_t< right_type_v<T>>;

    template<typename T>
    T const& widthDefault(T* pt, T const& inDefault)
    {
        return pt ? *pt : inDefault;
    }

    template<typename T, typename = check_t<T> >
    T const& get(i32 index, T const& inDefault) {
        return widthDefault<T>( std::get_if<T>(&definedConfigs[index]), inDefault);
    }

    template<typename T, typename = check_t<T> >
    T const& get(String const& name, T const& inDefault) {
        auto& found = configs.find(name);
        if (found == configs.end()) {
            return nullptr;
        }
        return widthDefault<T>(std::get_if<T>(*found), inDefault) ;
    }

    template<typename T, typename = check_t<T>  >
    void set(i32 index, T const& t) {
        rs_check(index >= 0 && index < Max);
        definedConfigs[index] = t;
    }

    template<typename T, typename = check_t<T>  >
    void set(String const& name, T const& t) {
        configs.insert_or_assign(name, t);
    }

    template<typename T, typename = check_t<T>  >
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
    std::unordered_set<String>                   switches{};
    std::array< AppConfigVarient, AppConfigs::Max>    definedConfigs{};
    std::unordered_map<String, AppConfigVarient> configs{};
};
inline AppConfigs GAppConfigs;

PROJECT_NAMESPACE_END