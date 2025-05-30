#pragma once

#include <string>
#include <memory>
#include <vector>

class IDevice
{
public:
    static std::shared_ptr<IDevice> createD3D12Device(const std::string& vendor);

    const std::string &getVendor()
    {
        return m_vendor;
    }

private:
    std::string m_vendor;
};