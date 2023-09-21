#pragma once
#if RS_PLATFORM_ANDROID

#include<stdio.h>
#include<unordered_map>
#include"../../type.h"
#include <android/log.h>

PROJECT_NAMESPACE_BEGIN

// 1. commandline input/start argument 
// 2. get from env prop
class AndroidGlobalArgments
{
    private:
    inline static std::unordered_map< std::string, int> _name2Ints{};
    inline static std::unordered_map< std::string, float> _name2Floats{};
    inline static std::unordered_map< std::string, std::string> _name2Strings{};

    static constexpr int32 kBufferSize = 256;

    static bool getProperty(const char* key, char* out, char* def = nullptr){
        rs_check(key!=nullptr);
        rs_check(out!=nullptr);
        std::string cmd = "getprop debug.vk_rs_debug_layer.";
        cmd += key;
        FILE* file = popen(cmd.c_str(), "r");
        if(!file ||  ferror(file)) {
            //__android_log_print(4, "vk_rs_debug_layer UE4", "open pipe failed %s", cmd.c_str() );
            return false;
        }
        int n = fread(out, kBufferSize-1, 1, file);
        if(ferror(file)) {
            if(def) {
                strcpy(out, def);
            }
            return false;
        }
        //__android_log_print(4, "vk_rs_debug_layer UE4", "open pipe succ %s", out );
        pclose(file);
        return out && out[0]!='\n';
    }
    public:
    static bool setProperty(char const* key, char const* value){
        rs_check(key!=nullptr);
        rs_check(value!=nullptr);
        ptr::size_t  n = strlen(value);
        if(n>0){
            std::string cmd = "setprop debug.vk_rs_debug_layer.";
            cmd += key;

            //__android_log_print(4, "vk_rs_debug_layer UE4", "cmd: %s %s", cmd.c_str(),value );
            
            FILE* file = popen(cmd.c_str(), "we");
            if(!file ||  ferror(file)) {
                __android_log_print(4, "vk_rs_debug_layer UE4", "open pipe failed %s", cmd.c_str() );
                return false;
            }
            
            ptr::size_t nret = fwrite(value, n, 1, file);
            if(nret!=n){
                __android_log_print(4, "vk_rs_debug_layer UE4", "write value:%s len:%d fail of return len:%d", value, (int)n, (int)nret );
            }
            pclose(file);
            return nret == n;
        }
        return true;
    }



    static int getInt(std::string const& key, int def=0){
        auto found = _name2Ints.find(key);
        if(found!=_name2Ints.end()){
            return found->second;
        }
        int value = def;
        char outbuffer[kBufferSize]{0};
        if(getProperty(key.c_str(), outbuffer) && outbuffer[0]!=0 ){
            value = std::stoi(outbuffer);
            _name2Ints.emplace(key, value);
        }
        return value;
    }

    static float getFloat(std::string const& key, float def=0){
        auto found = _name2Floats.find(key);
        if(found!=_name2Floats.end()){
            return found->second;
        }
        float value = def;
        char outbuffer[kBufferSize]{0};
        if(getProperty(key.c_str(), outbuffer) && outbuffer[0]!=0 ){
            value = std::stof(outbuffer);
            _name2Floats.emplace(key, value);
        }
        return value;
    }

    static std::string const& getString(std::string const& key, std::string const& def=""){
        auto found = _name2Strings.find(key);
        if(found!=_name2Strings.end()){
            return found->second;
        }
        char outbuffer[kBufferSize]{0};
        if(getProperty(key.c_str(), outbuffer) && outbuffer[0]!=0){
            auto&& result = _name2Strings.emplace(key, outbuffer);
            std::string& value = result.first->second;
            value.erase(value.find_last_not_of(" \n\r\t") + 1);
            return value;
        }
        return def;
    }

    // flush the cache
    static void flush()
    {
        _name2Ints.clear();
        _name2Floats.clear();
        _name2Strings.clear();
    }
};

using GlobalArgments = AndroidGlobalArgments;

PROJECT_NAMESPACE_END

#endif