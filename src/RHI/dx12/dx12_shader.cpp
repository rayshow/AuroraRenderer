#include "dx12_shader.h"

#include"HAL/filesystem.h"

PROJECT_NAMESPACE_BEGIN

bool DX12Shader::loadFromFile(WString const& path)
{
    File file;
    if(file.open(path, EFileOption::Read))
    {
        u64 size = file.size();
        std::vector<u8> data(size);
        if( !AREnsure( file.read(data)))
        {
            AR_LOG(Error, "error load data from file %ls because: %s", path.c_str(), file.getError());
            return false;
        }
    }else
    {
        AR_LOG(Error, "error open file %ls because: %s", path.c_str(), file.getError());
    }
}




PROJECT_NAMESPACE_END