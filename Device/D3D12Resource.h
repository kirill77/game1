#pragma once

#include "IResource.h"
#include "D3D12Device.h"
#include <d3d12.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

class D3D12Resource : public IResource
{
public:
    D3D12Resource(ComPtr<ID3D12Resource> resource);

    // IResource interface
    virtual void loadFromFile(const std::filesystem::path& sPath, IQueue* pQueue) override;
    virtual void getDesc(ResDesc& outDesc) override;
    virtual void writeTo(const char* pData, uint32_t nBytes) override;

    // Getter for the underlying D3D12 resource
    ID3D12Resource* getResource() const { return m_resource.Get(); }

    // Set a name for the resource (useful for debugging)
    virtual void setName(const std::wstring& name) override {
        if (m_resource) {
            m_resource->SetName(name.c_str());
        }
    }

private:
    ComPtr<ID3D12Resource> m_resource;
}; 