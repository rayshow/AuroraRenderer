#include "SHA_hash.h"
#include "sha1.h"
#include "HAL/assert.h"

PROJECT_NAMESPACE_BEGIN

class SHAHasherImpl : public SHAHasher
{
public:
    SHA1 shaAlg{};
    virtual void add(u8 const* buffer, usize size) override
    {
        shaAlg.add(buffer, size);
    }
    virtual void get(u8 (&bytes)[20]) override 
    {
        shaAlg.getHash(bytes);
    }
};

TUniquePtr<SHAHasher> SHAHasher::create()
{
    return std::make_unique<SHAHasherImpl>();
}

SHAHash SHAHash::hash(void const* buffer, usize size)
{
    SHA1 shaAlg{};
    SHAHash result{};
    shaAlg.add(buffer, size);
    shaAlg.getHash(result.hashBytes);
    return result;
}

template<typename Char, bool bCapital>
Char ByteToHex(u8 number)
{
    if constexpr (std::is_same_v<Char, char>)
    {
        u8 gap = bCapital ? 'A' : 'a' - 10;
        return number + number > 9 ? gap : '0';
    }else
    {
        u8 gap = bCapital ? L'A' : L'a' - 10;
        return number + number > 9 ? gap : L'0';
    }
}

template<typename Char>
u8 HexToByte(Char c)
{
    if (c >= '0' && c <= '9')
    {
        return u8(c - '0');
    }
    if (c >= 'A' && c <= 'F')
    {
        return u8(c - 'A' + 10);
    }
    if (c >= 'a' && c <= 'f')
    {
        return u8(c - 'a' + 10);
    }
}

template<typename Str, ArCheckType(Str, is_string)>
void SHAHash::toString(Str& str)
{
    using CharType = typename Str::value_type;
    for (u32 i = 0; i < kShaByteLength; ++i)
    {
        u8 byte = hashBytes[i];
        u8 high = byte >> 4;
        u8 low = byte & 0x0F;
        str.append(ByteToHex<CharType,true>(high));
        str.append(ByteToHex<CharType,true>(low));
    }
}

template<typename Str, ArCheckType(Str, is_string)>
void SHAHash::fromString(Str const& str)
{
    using CharType = typename Str::value_type;
    CharType const* begin = str.data();
    if(AREnsure(str.length() >= kStringLength))
    {
        for(u32 i = 0; i < kShaByteLength; ++i)
        {
            u8 high = HexToByte(begin++);
            u8 low = HexToByte(begin++);
            hashBytes[i] = high << 4 | low;
        }
    }
}

PROJECT_NAMESPACE_END