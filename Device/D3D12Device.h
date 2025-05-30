#pragma once

#include "IDevice.h"
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl/client.h>

using Microsoft::WRL::ComPtr;

struct D3D12Device : public IDevice
{
    D3D12Device(bool bUseIntegratedGpu);

    // Getters for device and factory
    ID3D12Device* getDevice() const { return m_pDevice.Get(); }
    IDXGIFactory6* getFactory() const { return m_pDxgiFactory.Get(); }

    // IDevice interface
    virtual std::shared_ptr<IWindow> createWindow(uint32_t nSwapChainImages) override;
    virtual std::shared_ptr<IQueue> createQueue(const std::wstring &sName) override;
    virtual std::shared_ptr<IResource> createResource(const IResource::ResDesc& desc) override;
    virtual std::shared_ptr<IResource> createSharedResource(std::shared_ptr<IDevice> pOtherDevice, std::shared_ptr<IResource> pResource) override;
    virtual std::shared_ptr<IFence> createFence() override;

private:
    ComPtr<ID3D12Device> m_pDevice;
    ComPtr<IDXGIFactory6> m_pDxgiFactory;
};