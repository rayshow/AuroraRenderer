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
        WinWidth,
        WinHeight,
        WinResizable,
        VSync,
        ProjectName,
        Max = 32,
    };
    using AppConfigVarient = std::variant<i64, f64, i32, f32, bool, void*, std::string>;
    template<typename T>
    using right_type = is_one_of<T, i64, f64, i32, f32, bool, void*, std::string>;
    template<typename T>
    static constexpr bool right_type_v = right_type<T>::value;
    template<typename T>
    using check_t = std::enable_if_t< right_type_v<T>>;

    template<typename T>
    T& widthDefault(T* pt, T const& inDefault)
    {
        return pt ? *pt : inDefault;
    }

    template<typename T, typename = check_t<T> >
    T& get(i32 index, T const& inDefault) {
        return widthDefault<T>( std::get_if<T>(&definedConfigs[index]), inDefault);
    }

    template<typename T, typename = check_t<T> >
    T& get(std::string const& name, T const& inDefault) {
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
    void set(std::string const& name, T const& t) {
        auto& result = configs.emplace(name, t);
        if (!result.second) {
            *result.first = t;
        }
    }

private:
    std::unordered_set<std::string>                   switches{};
    std::array< AppConfigVarient, AppConfigs::Max>    definedConfigs{};
    std::unordered_map<std::string, AppConfigVarient> configs{};
};
inline AppConfigs GAppConfigs;

PROJECT_NAMESPACE_END