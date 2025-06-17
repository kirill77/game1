#pragma once

#include <filesystem>
#include <memory>
#include <array>

struct IDevice;
struct IQueue;

struct IResource : public std::enable_shared_from_this<IResource>
{
    enum eFormat
    {
        eFormatUnknown = 0,
        eFormatRGBA8 = 1
    };
    static uint32_t getBytesPerPixel(eFormat format);

    struct ResDesc
    {
        eFormat m_format = eFormatRGBA8;
        uint32_t m_nDims = 0;
        std::array<uint32_t, 3> m_res;
        bool m_isStaging = false;
        bool m_isShared = false;

        inline bool operator ==(const ResDesc& other) const
        {
            // TODO: implement
            return true;
        }
    };

    virtual void loadFromFile(const std::filesystem::path& sPath, IQueue* pQueue) = 0;
    virtual void getDesc(ResDesc &outDesc) = 0;
    virtual void writeTo(const char* pData, uint32_t nBytes) = 0;
    virtual void setName(const std::wstring& name) = 0;
};
