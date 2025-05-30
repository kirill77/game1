#pragma once

#include <string>
#include <memory>
#include <vector>
#include "math/vector.h"
#include "IResource.h"

struct IWindow;
struct IQueue;
struct IResource;
struct IFence;

struct IDevice : public std::enable_shared_from_this<IDevice>
{
    static std::shared_ptr<IDevice> createD3D12Device(bool bUseIntegratedGpu);

    virtual std::shared_ptr<IWindow> createWindow(uint32_t nSwapChainImages) = 0;
    virtual std::shared_ptr<IQueue> createQueue(const std::wstring &sName) = 0;
    virtual std::shared_ptr<IResource> createResource(const IResource::ResDesc& desc) = 0;
    virtual std::shared_ptr<IResource> createSharedResource(std::shared_ptr<IDevice> pOtherDevice, std::shared_ptr<IResource> pResource) = 0;
    virtual std::shared_ptr<IFence> createFence() = 0;

    inline const std::wstring& getDesc() const { return m_sDesc; }

protected:
    std::wstring m_sDesc;
};